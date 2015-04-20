/***************************************************
  Conservatory >> Kitchen Fan Controller
  
  13/4/2015 - First draft, using Adafruit display
  14/4/2015 - Combined ADC sample smoothing into one loop. Added upper temperature for Kitchen; above this the fan is disabled
  16/4/2015 - Added analogWrite to control the fan via PWM
 ****************************************************/
 //
#include <PID_v1.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9340.h"

#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// These are the pins used for the UNO
// for Due/Mega/Leonardo use the hardware SPI pins (which are different)
#define _sclk 52 
#define _miso 50
#define _mosi 51
#define _cs 53
#define _dc 9
#define _rst 8
//
//Set up Pin numbers
//
const int conservPin = 10;
const int kitchenPin = 9;
const int fanOutputPin = 12; // NEED TO CHECK THIS
//
//Initialise variables
//
int conservADCValue = 0;
int kitchenADCValue = 0;
double conservTemp = 0;
double kitchenTemp = 0;
double tempDiff = 0;
double kitchenSetpoint = 18.00; // Temperature at which the fan is stopped completely
double Setpoint = 0.5; //PID SetPoint - i.e. the value of tempDiff that the algorithm tries to maintain
const double fanRateParameter = 50; //Proportional value used to cal fan PWM output based on tempDiff
double fanPWMOutput = 0;
double fanPercent = 0;
int count = 0; //loop counter
const int smoothNo = 5; // Number of loops used in sensor smoothing
const int smoothTime = 100; // mS delay between each smoothing ADC sample
//
// Set Up Averaging Variables (stolen from Destrat code)
// 
const unsigned nRecentDiffs = 40; //Number of recent tempDiff values to store. MUST BE EVEN.
float recentDiffs[nRecentDiffs]; // Create array to store tempDiff readings
unsigned recentDiffsIndex = 0; // initialise pointer to tempDiff array
unsigned recentDiffsInterval = 2000; // time between successive tempDiff readings (ms)
double newAverageDiff = 0.0f; // variable to store calculated value of most recent tempDiff average
float oldAverageDiff = 0.0f; // variable to store calculated value of earlier tempDiff average
float tempDiffGradient = 0.0f; // variable to store calculated rate of change in tempDiff over time 
const float deltaTime = (float)nRecentDiffs * 0.5f * (float)recentDiffsInterval; // time between old and new average tempDiff measurements - used to calculate the gradient
unsigned elapsedMillis = 0; // to allow millis function to be used to time sampling rate
unsigned newMillis = 0; // as above
//
  // Function to ConvertADC Values to Degrees C
  //
float calcTempFromReadValue(int readValue);
//
// Using software SPI is really not suggested, its incredibly slow
//Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
// Use hardware SPImyPID.SetMode(AUTOMATIC); // turn on the PID loop
  
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);
// initialize the PID Loop
PID myPID(&newAverageDiff, &fanPWMOutput, &Setpoint,50,1,5, REVERSE);
//
void setup() {
  Serial.begin(9600);
  while (!Serial);
//  
  Serial.println("FAN CONTROL SETTING UP"); 
  delay(1000);
  tft.begin();
  tft.setRotation(3);
  //
  Serial.println(F("Done!"));
  myPID.SetOutputLimits(0, 254); 
  myPID.SetMode(AUTOMATIC); // turn on the PID loop
  myPID.SetSampleTime(5000); // from PID library - sets sample time to X milliseconds
  //
  for(unsigned i = 0; i < nRecentDiffs; ++i) // Initiatise array for storing temperature history (used in rolling average calculation)
  {
    recentDiffs[i] = 0.0f;
  }
}
void loop(){
  // Obtain smoothed Conservatory & Kitchen Sensor ADC values
  conservADCValue = 0;
  kitchenADCValue = 0;
  while (count < smoothNo){
  conservADCValue = conservADCValue + analogRead(conservPin);
  kitchenADCValue = kitchenADCValue + analogRead(kitchenPin);
  count = count + 1;
  delay(smoothTime);
  }
  conservADCValue = conservADCValue / smoothNo;
  kitchenADCValue = kitchenADCValue / smoothNo;
  count = 0;
  //
  conservTemp = calcTempFromRead(conservADCValue);
  kitchenTemp = calcTempFromRead(kitchenADCValue);
  //
  tempDiff = conservTemp - kitchenTemp;
  //*********************************************************************
  newMillis = millis();
  if(newMillis - elapsedMillis > recentDiffsInterval)
  {
    recentDiffs[recentDiffsIndex] = tempDiff;
    recentDiffsIndex = (recentDiffsIndex + 1) % nRecentDiffs;
    newAverageDiff = oldAverageDiff = 0.0f;
    for(unsigned i = recentDiffsIndex; i < recentDiffsIndex + (nRecentDiffs / 2); ++i)
    {
      oldAverageDiff += recentDiffs[i % nRecentDiffs];
    }
    for(unsigned i = recentDiffsIndex + (nRecentDiffs / 2); i < recentDiffsIndex + nRecentDiffs; ++i)
    {
      newAverageDiff += recentDiffs[i % nRecentDiffs];
    }
    newAverageDiff /= (float)nRecentDiffs * 0.5f;
    oldAverageDiff /= (float)nRecentDiffs * 0.5f;
    tempDiffGradient = (newAverageDiff - oldAverageDiff)/deltaTime;
    
   // hoursUntilFull = (maxEnergy - energy) / (energyGradient * 3600000.0f);
    elapsedMillis = newMillis;
 //   
    //Print out array
    Serial.print("Average tempDiff = ");
    Serial.println(newAverageDiff);
    Serial.println("Recent tempDiff levels");
    for(unsigned i = 0; i < nRecentDiffs-1; ++i)
    {
      Serial.print(recentDiffs[i]);
      Serial.print(" ");
    }
    Serial.println(" ");
  }
  // End of section to calculate rolling average tempDiff
  //*********************************************************************
  myPID.Compute();
  if (kitchenTemp < kitchenSetpoint)
  {
      analogWrite(fanOutputPin, fanPWMOutput);
      fanPercent = (fanPWMOutput * 100) / 254;
      if (fanPWMOutput > 253){
        analogWrite(fanOutputPin, fanPWMOutput);
        fanPercent = (fanPWMOutput * 100) / 254;
        tft.fillScreen(ILI9340_RED);
        tft.setTextSize(3);
        tft.setCursor(15,80);
        tft.setTextColor(ILI9340_WHITE);
        tft.println("Delta-T OVER MAX");
        tft.println(" ");
        tft.println("   FAN AT 100%");
        delay(4000);
      }
      else
      { 
        analogWrite(fanOutputPin, fanPWMOutput);
        fanPercent = (fanPWMOutput * 100) / 254;
      }
   }
  else // kitchenTemp is above maximum allowed so turn Fan OFF
  {
    fanPWMOutput = 0; 
    analogWrite(fanOutputPin, fanPWMOutput);
    fanPercent = (fanPWMOutput * 100) / 254;
    // Display Status
    tft.fillScreen(ILI9340_RED);
    tft.setTextSize(3);
    tft.setCursor(4,60);
    tft.setTextColor(ILI9340_WHITE);
    tft.println("   FAN IS OFF");
    tft.println(" ");
    tft.println("   Kitchen Temp");
    tft.println("   Exceeds");
    tft.println("   SetPoint of");
    tft.print("   ");
    tft.print(kitchenSetpoint);
    tft.print(" C");
    delay(4000);
 }
  // Display Current Temps and Fan Speed
  tft.fillScreen(ILI9340_BLACK);
  tft.setCursor(0, 10);
  tft.setTextColor(ILI9340_WHITE);  tft.setTextSize(3);
  tft.print("Conserv. ");
  tft.print(conservTemp);
  tft.println(" C");
  tft.println("");
  //
  tft.setCursor(0, 70);
  tft.setTextColor(ILI9340_YELLOW);  tft.setTextSize(3);
  tft.print("Kitchen  ");
  tft.print(kitchenTemp);
  tft.println(" C");
  tft.println("");
  //
  tft.setCursor(0, 130);
  tft.setTextColor(ILI9340_GREEN);  tft.setTextSize(3);
  tft.print("Delta-T  ");
  tft.print(newAverageDiff);
  tft.println(" C");
  tft.println("");
  //
  tft.setCursor(0, 190);
  tft.setTextColor(ILI9340_YELLOW);  tft.setTextSize(5);
  tft.print("Fan ");
  tft.print(fanPercent);
  tft.print("% ");
  //tft.print(fanPWMOutput);
  //  
 // delay(5000);

} // END OF MAIN LOOP
//
// Function to ConvertADC Values to Degrees C
//
  float calcTempFromRead(int readValue) {
    return (((float)readValue * 500.0f) / 1024.0f)-47;
  }
