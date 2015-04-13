/***************************************************
  Conservatory >> Kitchen Fan Controller
  
  13/4/2015 - First draft, using Adafruit display
 ****************************************************/
 //
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
const double fanRateParameter = 50; //Proportional value used to cal fan PMW output based on tempDiff
int fanPMWOutput = 0;
int fanPercent = 0;
int count = 0; //loop counter
const int smoothNo = 20; // Number of loops used in sensor smoothing
const int smoothTime = 100; // mS delay between each smoothing ADC sample
//
  // Function to ConvertADC Values to Degrees C
  //
float calcTempFromReadValue(int readValue);
//
// Using software SPI is really not suggested, its incredibly slow
//Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _mosi, _sclk, _rst, _miso);
// Use hardware SPI
Adafruit_ILI9340 tft = Adafruit_ILI9340(_cs, _dc, _rst);

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
}
void loop(){
  // Obtain smoothed Conservatory Sensor ADC value
  conservADCValue = 0;
  while (count < smoothNo){
  conservADCValue = conservADCValue + analogRead(conservPin);
  count = count + 1;
  delay(smoothTime);
  }
  conservADCValue = conservADCValue / smoothNo;
  count = 0;
  // Obtain smoothed Kitchen Sensor ADC value
  kitchenADCValue = 0;
  while (count < smoothNo){
  kitchenADCValue = kitchenADCValue + analogRead(kitchenPin);
  count = count + 1;
  delay(smoothTime);
  }
  kitchenADCValue = kitchenADCValue / smoothNo;
  count = 0;
  //
  conservTemp = calcTempFromRead(conservADCValue);
  kitchenTemp = calcTempFromRead(kitchenADCValue);
  tempDiff = conservTemp - kitchenTemp;
  if(tempDiff > 0)
  {
    fanPMWOutput = tempDiff * fanRateParameter;
    if (fanPMWOutput > 255){
    fanPMWOutput = 255;
    tft.setCursor(0,190);
    tft.setTextColor(ILI9340_BLUE);
    tft.print("                ");
    tft.setCursor(0,190);
    tft.setTextColor(ILI9340_RED);
    tft.print("Delta-T OVER MAX");
    delay(1000);
    tft.setCursor(0,190);
    tft.print("                ");
    }
    fanPercent = (fanPMWOutput * 100) / 255;
  }
  else
  {
    fanPMWOutput = 0;
  }
  //
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
  tft.print(tempDiff);
  tft.println(" C");
  tft.println("");
  //
  tft.setCursor(0, 190);
  tft.setTextColor(ILI9340_BLUE);  tft.setTextSize(3);
  tft.print("Fan ");
  tft.print(fanPercent);
  tft.print("% ");
  //tft.print(fanPMWOutput);
  //  
  delay(5000);

} // END OF MAIN LOOP

// Function to ConvertADC Values to Degrees C
  //
  float calcTempFromRead(int readValue) {
    return (((float)readValue * 500.0f) / 1024.0f)-47;
  }
