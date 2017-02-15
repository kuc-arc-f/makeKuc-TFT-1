/*
 TFT LCD (ILI9341)+ analog(LM60BIZ)
 NTP clock, sensor value display
 with mkuBord-45
*/
#include <SoftwareSerial.h>
#include "SPI.h"
#include <Wire.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Time.h>
#include <TimeLib.h> 

// For the Adafruit shield, these are the default.
//#define TFT_RST 8
//#define TFT_DC 9
#define TFT_RST 2
#define TFT_DC 3

#define TFT_CS 10
// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

//define
#define TIME_MSG_LEN  11   // time sync to PC is HEADER followed by Unix time_t as ten ASCII digits
#define TIME_HEADER  'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message 
// T1262347200  //noon Jan 1 2010
SoftwareSerial mySerial(5, 6); /* RX:D5, TX:D6 */
//
const String mVersion="0.9.1";
const int mVoutPin = 0;
int mTemp=0;
uint32_t mTimerTmp;
uint32_t mTimerTime;
uint32_t mReceive_Start=0;
String mTimeStr = "";
//
void setup() {
  Serial.begin(9600);
  mySerial.begin( 9600 );
  Serial.println("# ILI9341 Test! -0214a12"); 
  pinMode(mVoutPin, INPUT);
  tft.begin();

  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(ILI9341_RDMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDPIXFMT);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDIMGFMT);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  
  Serial.println(F("Benchmark                Time (microseconds)"));
  delay(10);
//  Serial.print(F("Screen fill              "));
//  Serial.println(testFillScreen());
//  delay(500);
  //
  uint8_t rotation= 1;
  tft.setRotation(rotation);
  //tft.fillScreen(ILI9341_WHITE);
  //
  Serial.println(F("Done!"));
}

uint32_t mTimer=0;
uint32_t mCounter=0;

long convert_Map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
// reading LM60BIZ
int getTempNum(){
  int iRet=0;
  float fSen  = 0;
  unsigned long reading  = 0;
  int iDiv =3;  
  for (int i=0; i< iDiv; i++) {
    int  iTmp = analogRead(mVoutPin);
    reading  += iTmp; 
    delay(50);
  }
  int SValue= reading / iDiv;
  int voltage=convert_Map(SValue, 0, 1000, 0,3300);  // V
//Serial.print("SValue=");
//Serial.print(SValue);
//Serial.print(" , voltage=");
//Serial.println(voltage);
  int iTemp = (voltage - 424) / 6.25; // offset=425
  iRet= iTemp;
  
  return iRet;  
}
//
boolean Is_validHead(String sHd){
  boolean ret=false;
  if(sHd=="d1"){
    ret= true;
  }else if(sHd=="d2"){
    ret= true;
  }
  return ret;
}
String  digitalClockDisplay(){
  String sRet="";
  // digital clock display of the time
  char cD1[8+1];
  time_t t = now();
  const char* fmtSerial = "%02d:%02d:%02d";
  sprintf(cD1, fmtSerial, hour(t), minute(t), second(t));
//Serial.println(cD1);
  sRet=String(cD1);
  return sRet;  
}
//
void conv_timeSync(String src){
  //String sRet="";
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 
   pctime = src.toInt();
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   }  
   //return sRet;
}
String mBuff="";
//
void proc_uart(){
    while( mySerial.available() ){
      char c= mySerial.read();
      mBuff.concat(c );
      if(  mBuff.length() >= 13 ){
        String sHead= mBuff.substring(0,2);
        boolean bChkHd= Is_validHead( sHead );
        if(bChkHd== true){
// Serial.println( "Hd="+ sHead );
Serial.println( "mBuff="+ mBuff );
          String sTmp= mBuff.substring(3,13);
          conv_timeSync(sTmp);
           //send
           int iD1=int(mTemp );
           char cD1[10+1];
           sprintf(cD1 , "d1%08d", iD1);       
           mySerial.print( cD1 );
           mReceive_Start=millis();
//Serial.print("cD1=");
//Serial.println(cD1  );                    
        }        
        mBuff="";
      }else{
          if(mReceive_Start > millis()+ 10000 ){
            mBuff="";
          }
      }
    } //end_while
}
//
void loop(void) {
  proc_uart();
  if(millis() >mTimer ){
    mTimer=millis() + (5 * 1000) ;
    mTemp = getTempNum();
    mTimeStr= digitalClockDisplay();
    testText(mTimeStr );
    mCounter++;
  }
}
//
//unsigned long testText() {
unsigned long testText(String sTime) {
  tft.fillScreen(ILI9341_WHITE);
  unsigned long start = micros();
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(20, 20);
  tft.setTextSize(2);
  tft.println("IoT Clock, v"+ mVersion  );
  //
  tft.setTextSize(5);
  tft.println("");
  tft.println(" " +sTime );
  tft.setTextSize(4);
  tft.println("");
  tft.setTextSize(6);
  tft.setTextColor(ILI9341_BLUE );
  tft.println(" T:"+ String(mTemp ) +"C");
  tft.setTextColor(ILI9341_BLACK);
//  tft.setTextSize(2);
//  tft.println("");
//  tft.println(" by make-KUC");
  return micros() - start;
}
//
unsigned long testFillScreen() {
  unsigned long start = micros();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  tft.fillScreen(ILI9341_RED);
  yield();
  tft.fillScreen(ILI9341_GREEN);
  yield();
  tft.fillScreen(ILI9341_BLUE);
  yield();
  tft.fillScreen(ILI9341_BLACK);
  yield();
  return micros() - start;
}






