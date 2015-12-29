#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <NewPing.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <OneWire.h>
#include <Time.h>
#include <Timer.h>

// SET IS_DEBUG_BUILD to 0 TO REMOVE ALL SERIAL AND DEBUG FUNCTIONALITY

#define IS_DEBUG_BUILD  1
#if IS_DEBUG_BUILD == 1
  #define DEBUG_BUILD
#endif

// Arduino Pin definitions
#define HEATER_RELAY_PIN      7
#define ATO_RELAY_PIN         8
#define FEEDMODE_RELAY_PIN    9
#define ECHO_PIN             10
#define TRIGGER_PIN          11
#define ONE_WIRE_PIN         12
#define TOUCH_SENSOR_PIN     53
#define MAX_DISTANCE        200
#define imagedatatype  unsigned int

// CONSTS FOR UTFT DISPLAY PAGES
const byte HOME = 0;
const byte WATER_PARAMS = 1;
const byte DEBUG = 2;
const byte DEBUG2 = 3;
const byte WIFI_SETUP = 4;
const byte SETTINGS = 5;
const byte SET_TIME = 6;

// AVAILABLE UTFT FONTS
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t SevenSegmentFull[];
extern uint8_t Inconsola[];
extern uint8_t SevenSegNumFont[];
extern uint8_t various_symbols[];
extern uint8_t Dingbats1_XL[];

// Declare which bitmap buttton we will be using
//extern imagedatatype cat[];
//extern imagedatatype monkey[];

// SYSTEM VARS
const int EST = 18000;
bool IsOnFeedMode = false;
int CurrentWaterLevel = 0;
int CurrentDisplayPage = HOME;
int CurrentDebugPageYPos = 0;
int feedButton, settingsButton, debugButton;
int backButton, prevPageButton, nextPageButton;
int pressed_button;

// USER CONFIG OPTIONS
bool ShowDebugInformation = true;
bool DisplayShortDate = false;
bool DisplayTimeAs24HRClock = false;
bool ShowTempInCelcius = true;
bool StartWithFeedModeOn = false;
float MaxTempThreshold = 28; // IN CELCIUS
float MinTempThreshold = 22; // IN CELCIUS
byte FeedTimeInMin = 3;
byte MinWaterLevel = 6;
const byte MaxWaterLevel = MinWaterLevel - 2;

// DECLARE ALL OBJECTS
Timer t;
OneWire ds(ONE_WIRE_PIN);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
UTFT myGLCD(SSD1289, 38, 39, 40, 41);
UTouch  myTouch(6,5,4,3,2);
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

void setup()
{
  Serial.begin(115200);
  Serial.setTimeout(5000); 
  loadConfigFromMemory(); 
  Serial.println("");
  Serial.println("---- Start Setup ---- ");
  
  //TODO: CONVERT TO RTC
  setTime(1450512764 - EST);
  
  /*  
  String output = "";
  char[] currentTime =  (char[])getTime();
  output = sprintf("----  Init Manual Time - %c ----", currentTime);
  Serial.println(output);
  */
  Serial.println("----  Init Manual Time - " + (String)getTime() + " ----");
 
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(ECHO_PIN, LOW);
  setCurrentWaterLevel();
  Serial.println("----  Init UC - WL: " + (String)CurrentWaterLevel + " ----");

  pinMode(TOUCH_SENSOR_PIN, INPUT);
  Serial.println("----  Init TOUCH SENSOR: " + (String)digitalRead(TOUCH_SENSOR_PIN) + " ---- ");

  pinMode(HEATER_RELAY_PIN, OUTPUT);
  digitalWrite(HEATER_RELAY_PIN, LOW);
  pinMode(ATO_RELAY_PIN, OUTPUT);
  digitalWrite(ATO_RELAY_PIN, HIGH);
  pinMode(FEEDMODE_RELAY_PIN, OUTPUT);
  digitalWrite(FEEDMODE_RELAY_PIN, LOW);
  Serial.println("----  Init Relay (ATORelay: " + (String)digitalRead(ATO_RELAY_PIN) + ", FeedModeRelay: " + (String)digitalRead(FEEDMODE_RELAY_PIN) + ") ---- ");

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setBackColor(VGA_BLACK);
  myGLCD.setColor(VGA_WHITE);
  Serial.println("----  Init Display ----");  

//  myTouch.InitTouch();
//  myTouch.setPrecision(PREC_MEDIUM);
  myButtons.setSymbolFont(Dingbats1_XL);
  Serial.println("----  Init Touch Display ----");  

  #if defined(DEBUG_BUILD)
    if(ShowDebugInformation) {t.every(30000, showDebugInfo);}
  #endif
  t.every((long)(60000 * 15), logData);
  t.every(10000, updateDateTime);
  t.every(7000, setCurrentWaterLevel);    
  t.every(1200, updateDisplay);
  t.every(1000, checkFeedMode);
  t.every(100, checkButtonPressed);  
  Serial.println("----  Init Events ----");
  
  Serial.println("---- End SetUp ---- ");
  Serial.println("");
 
  updateDateTime();
  #if defined(DEBUG_BUILD)
    showDebugInfo();  
  #endif
  updateDisplay();
  addButtons();
}

void loop()
{ 
  t.update();
}

#if defined(DEBUG_BUILD)
void showDebugInfo()
{
  Serial.println("---- DEBUG INFO ----");
  Serial.println("PUMP_RELAY_PIN: " + (String)digitalRead(ATO_RELAY_PIN));
  Serial.println("FEEDMODE_RELAY_PIN: " + (String)digitalRead(FEEDMODE_RELAY_PIN));
  Serial.println("HEATER_RELAY_PIN: " + (String)digitalRead(HEATER_RELAY_PIN)); 
  Serial.println("TOUCH_SENSOR_PIN: " + (String)digitalRead(TOUCH_SENSOR_PIN));  
  Serial.println("MinCurrentWaterLevel: " + (String)MinWaterLevel);
  Serial.println("MaxCurrentWaterLevel: " + (String)MaxWaterLevel);
  Serial.println("ShowTempInCelcius: " + (String)ShowTempInCelcius);
  Serial.println("DisplayTimeAs24HRClock: " + (String)DisplayTimeAs24HRClock);
  Serial.println("DisplayShortDate: " + (String)DisplayShortDate);
  Serial.println("CurrentDisplayPage: " + CurrentDisplayPage);
  Serial.println("MaxTempThreshold: " + (String)MaxTempThreshold);  
  Serial.println("MinTempThreshold: " + (String)MinTempThreshold);  
  Serial.println("FeedTimeInMin: " + (String)FeedTimeInMin);  
  Serial.println("MinWaterLevel: " + (String)MinWaterLevel);  
  Serial.println("MaxWaterLevel: " + (String)MaxWaterLevel);  
  Serial.println("---------");
  Serial.println("Time: " + getTime() +  (!DisplayTimeAs24HRClock ? getAMPM() : ""));
  Serial.println("Date: " + getDate());
  Serial.println("IsOnFeedMode: " + (String)(IsOnFeedMode ? "YES" : "NO"));  
  Serial.println("StartWithFeedModeOn: " + (String)(StartWithFeedModeOn ? "YES" : "NO"));      
  Serial.println("Temp: " + (String)getTemp() + " - (MaxTempC: " + MaxTempThreshold + ")");
  Serial.println("CurrentWaterLevel: " + (String)CurrentWaterLevel + " - MinWL: " + MinWaterLevel);
  Serial.println("ATO: " + (String)(digitalRead(ATO_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (CurrentWaterLevel > MinWaterLevel ? "ON " : "OFF"));
  Serial.println("RTPump: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (String)(IsOnFeedMode ? "OFF" : "ON ")); 
  Serial.println("Skimmer: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (String)(IsOnFeedMode ? "OFF" : "ON "));
  Serial.println("Heater: " + (String)(digitalRead(HEATER_RELAY_PIN) ? "OFF" : "ON ") +  " - Should Be: " + (String)(getTemp() < MaxTempThreshold ? "ON " : "OFF"));
  Serial.println("----------------");
  Serial.println("");  
}
#endif

void checkButtonPressed() 
{
  /*
  if (myTouch.dataAvailable() == true)
  {
    switch(myButtons.checkButtons())
    {
      case feedButton:
        myButtons.disableButton(feedButton);
        feedModeOn();
      break;
      case settingsButton:
      break;      
      case debugButton:
        myGLCD.setDisplayPage(DEBUG);
        CurrentDisplayPage = DEBUG;
        myGLCD.clrScr();
        displayDebugInfo();
        CurrentDebugPageYPos = 0;      
      break; 
      default:
        myGLCD.setDisplayPage(HOME);
        CurrentDisplayPage = HOME;
        myGLCD.clrScr();
        updateDateTime();
        updateDisplay();
        addButtons();   
     break;
    }
*/

  //SWITCH BETWEEN UTFT PAGES
  int _ctsValue = digitalRead(TOUCH_SENSOR_PIN);
  if(_ctsValue == HIGH)
  {
    switch (CurrentDisplayPage)
    {
      case HOME:
        digitalWrite(TOUCH_SENSOR_PIN, LOW);
        myGLCD.setDisplayPage(DEBUG);
        CurrentDisplayPage = DEBUG;
        myGLCD.clrScr();
        displayDebugInfo();
        CurrentDebugPageYPos = 0;
      break;
      case DEBUG:
        digitalWrite(TOUCH_SENSOR_PIN, LOW);      
        myGLCD.setDisplayPage(DEBUG2);
        CurrentDisplayPage = DEBUG2;
        myGLCD.clrScr();
        displayDebugInfo();
        CurrentDebugPageYPos = 0;
      case DEBUG2:
        digitalWrite(TOUCH_SENSOR_PIN, LOW);      
        myGLCD.setDisplayPage(HOME);
        CurrentDisplayPage = HOME;
        myGLCD.clrScr();
        updateDateTime();
        updateDisplay();
        addButtons();
      break;
    }
  }
}  

void displaySettingsPage() {}
void displaySetTimePage() {}
void displayWIFIPage() {}
void displayDebugPage() {}
void displayDebugPage2() {}

void updateDisplay()
{   
   if(CurrentDisplayPage != HOME)
    return;
    
  String _temp = getTempStr().substring(0, 4);
  myGLCD.setFont(BigFont);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.fillRoundRect(3, 80, 312, 85);

  //  START LEFT COLUMN 
  myGLCDPrint("Temp: " + _temp + (ShowTempInCelcius ? "C" : "F"), 5, 95, digitalRead(HEATER_RELAY_PIN) == HIGH);
  myGLCDPrint("Pump: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON "), 5, 115, digitalRead(FEEDMODE_RELAY_PIN) == HIGH);   
  myGLCDPrint("Skim: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON "), 5, 135, digitalRead(FEEDMODE_RELAY_PIN) == HIGH);
  myGLCDPrint("WLvl: " + (String)CurrentWaterLevel + " ", 5, 155);  

  //  START RIGHT COLUMN 
  myGLCDPrint("PH: -- ", 205, 95);  
  myGLCDPrint("Sg: -- ", 205, 115);  
  myGLCDPrint("Heat: " + (String)(digitalRead(HEATER_RELAY_PIN) ? "OFF" : "ON "), 174, 135, digitalRead(HEATER_RELAY_PIN) == HIGH);
  myGLCDPrint("ATO: " + (String)(digitalRead(ATO_RELAY_PIN) ? "OFF" : "ON "), 190, 155, digitalRead(ATO_RELAY_PIN) == LOW);
}

unsigned long Watch, _micro, time = micros();
unsigned int Clock = 0, R_clock;
boolean timerReset = false, timerStop = false, timerPaused = false;
volatile boolean timeFlag = false;

//Turns off Dual Outlet
void feedModeOn()
{ 
  if(CurrentDisplayPage != HOME)
    return;
    
  SetTimer(0,FeedTimeInMin,0);
  StartTimer();
  IsOnFeedMode = true;
  digitalWrite(FEEDMODE_RELAY_PIN, HIGH);   
  Serial.println("------ FEED MODE STARTED ----");
}

void checkFeedMode()
{ 
   if(CurrentDisplayPage != HOME)
    return;
    
  if(!IsOnFeedMode)
    return;
  
  CountDownTimer(); // run the timer
  if (TimeHasChanged())
  {
    if(ShowDebugInformation){Serial.println((String)ShowMinutes() + ":" + (String)ShowSeconds());}
    myGLCD.setFont(BigFont);
    myGLCDPrint((String)ShowMinutes() + ":" + printDigits(ShowSeconds()) + "        ", 60, 20);
  }
 
  if(timerStop)
  {
    digitalWrite(FEEDMODE_RELAY_PIN, LOW);
    IsOnFeedMode = false;
    myGLCDPrint("        ", 99, 185);
    myGLCDPrint("    ", 127, 205);
    myButtons.enableButton(feedButton);
    Serial.println("------ FEED MODE DONE --------"); 
  }
}

void myGLCDPrint(String str, int x, int y)
{
  myGLCDPrint(str, x, y, false);
}

void myGLCDPrint(String str, int x, int y, bool isError)
{  
  if(isError)
    myGLCD.setColor(VGA_RED);
  else
    myGLCD.setColor(VGA_WHITE);

  myGLCD.print(str, x, y);
}

// OUTPUT DEBUG INFO TO UTFT DEBUG MEMORY PAGE
 void displayDebugInfo()
{
/*
  Serial.println("MinCurrentWaterLevel: " + (String)MinWaterLevel);
  Serial.println("MaxCurrentWaterLevel: " + (String)MaxWaterLevel);
  Serial.println("MaxTempThreshold: " + (String)MaxTempThreshold);  
  Serial.println("MinTempThreshold: " + (String)MinTempThreshold);  
  Serial.println("FeedTimeInMin: " + (String)FeedTimeInMin);  
*/
   
  if(CurrentDisplayPage == DEBUG)
  {
    myGLCD.setFont(SmallFont);
    myGLCDPrintDebugInfo(DEBUG, "DEBUG INFO"); // @ " + getTime() + ":" + (String)second());   
    myGLCDPrintDebugInfo(DEBUG, "MinWaterLevel: " + (String)MinWaterLevel);
    myGLCDPrintDebugInfo(DEBUG, "MaxWaterLevel: " + (String)MaxWaterLevel);
    myGLCDPrintDebugInfo(DEBUG, "IsCelcius: " + (String)ShowTempInCelcius);
    myGLCDPrintDebugInfo(DEBUG, "24HRClock: " + (String)DisplayTimeAs24HRClock);
    myGLCDPrintDebugInfo(DEBUG, "ShortDate: " + (String)DisplayShortDate);
    myGLCDPrintDebugInfo(DEBUG, "PUMPRELAY: " + (String)digitalRead(ATO_RELAY_PIN));
    myGLCDPrintDebugInfo(DEBUG, "FEEDMODE: " + (String)digitalRead(FEEDMODE_RELAY_PIN));
    myGLCDPrintDebugInfo(DEBUG, "HEATER: " + (String)digitalRead(HEATER_RELAY_PIN)); 
    myGLCDPrintDebugInfo(DEBUG, "TOUCH: " + (String)digitalRead(TOUCH_SENSOR_PIN));  
  }
  
 if(CurrentDisplayPage != DEBUG2)
   return;

  myGLCD.setFont(SmallFont);   
  CurrentDebugPageYPos = 2;
  myGLCDPrintDebugInfo(DEBUG2, "Time: " + getTime() +  (!DisplayTimeAs24HRClock ? getAMPM() : ""));
  myGLCDPrintDebugInfo(DEBUG2, "Date: " + getDate());
  myGLCDPrintDebugInfo(DEBUG2, "Temp: " + (String)getTemp() + " - (MaxTempC: " + MaxTempThreshold + ")");
  myGLCDPrintDebugInfo(DEBUG2, "WaterLevel: " + (String)CurrentWaterLevel + " - MinWL: " + MinWaterLevel);
  myGLCDPrintDebugInfo(DEBUG2, "ATO: " + (String)(digitalRead(ATO_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (CurrentWaterLevel > MinWaterLevel ? "ON " : "OFF"));
  myGLCDPrintDebugInfo(DEBUG2, "RTPump: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (String)(IsOnFeedMode ? "OFF" : "ON ")); 
  myGLCDPrintDebugInfo(DEBUG2, "Skimmer: " + (String)(digitalRead(FEEDMODE_RELAY_PIN) ? "OFF" : "ON ") + " - Should Be: " + (String)(IsOnFeedMode ? "OFF" : "ON "));
  myGLCDPrintDebugInfo(DEBUG2, "Heater: " + (String)(digitalRead(HEATER_RELAY_PIN) ? "OFF" : "ON ") +  "- Should Be: " + (String)(getTemp() < MaxTempThreshold ? "ON " : "OFF"));
}

void myGLCDPrintDebugInfo(int debugPage, String str)
{  
  myGLCD.setWritePage(debugPage);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.print(str, 7, CurrentDebugPageYPos);
  CurrentDebugPageYPos += 15;
}

void updateDateTime()
{
  if(CurrentDisplayPage != HOME || IsOnFeedMode)
    return;
    
  myGLCD.setFont(BigFont); 
  myGLCDPrint(getDate(), 30, 0, 0);

  myGLCD.setFont(SevenSegmentFull); 
  myGLCDPrint(getTime(), 60, 20, 0);
  if(!DisplayTimeAs24HRClock)
  {
    String timeofDay = getAMPM();
    myGLCD.setFont(Inconsola);
    myGLCDPrint(timeofDay, 225, 40, 0);
  }
}

void setCurrentWaterLevel()
{
  int uS = sonar.ping();
  CurrentWaterLevel =  uS / US_ROUNDTRIP_CM;

  if(CurrentWaterLevel >= MinWaterLevel)
    digitalWrite(ATO_RELAY_PIN, LOW);
  else
    digitalWrite(ATO_RELAY_PIN, HIGH);
}

// TODO: Send To WS
void logData()
{
  String uploadData = (String)getTime() + "," + (String)getDate() + "," + (String)getTemp() + "," + (String)CurrentWaterLevel;
  Serial.println("");
  Serial.println("---- LOG DATA START ----");
  Serial.println(uploadData);
  Serial.println("---- LOG DATA COMPLETE ----");
  Serial.println("");  
}

String getTime() {
  if(DisplayTimeAs24HRClock)
  {
    String spacer = "";
    if(hour() < 10) { spacer = " ";}
    return spacer + (String)hour() + printDigits(minute());
  }
  else
  {
    int chour = hourFormat12();
    String spacer ="";
    if(chour < 10) {spacer = " ";}
    return spacer + (String)chour + ":" + printDigits(minute());
  }
}

String getAMPM()
{
  String amPM = "p";
  if(isAM()) {amPM = "a";}
  return amPM;
}

String getDate()
{
  String dString = (String)dayShortStr(weekday()) + ", " + (String)monthShortStr(month()) + " " + (String)day() + " " + (String)year();
  if(DisplayShortDate)
    dString = (String)month() + "/" + (String)day() + "/" + (String)(year() - 2000);
  return dString;
}

String getRandNumStr (int minNum, int maxNum)
{
  int val = random(minNum, maxNum);
  return (String)val;
}

int getRandNum (int minNum, int maxNum)
{
   return random(minNum, maxNum);
}

String printDigits(int digits)
{
  String sdigits = "";
  if (digits < 10)
    sdigits = "0";
  return sdigits + (String)digits;
}

String getTempStr()
{
  if(ShowTempInCelcius)
    return getCelsius();
  else
    return getFahrenheit();
}

String getFahrenheit()
{
  float ftemp = getTemp() * 1.8 + 32.0;
if(getTemp() < 0)
    return "ERR";
  return (String)ftemp + "F";
}

String getCelsius()
{
  if(getTemp() < 0)
    return "ERR";
  return (String)getTemp() + "C";
}

float getTemp()
{
 
byte data[12];
byte addr[8];

if ( !ds.search(addr)) {
//no more sensors on chain, reset search
ds.reset_search();
return -1000;
}

if ( OneWire::crc8( addr, 7) != addr[7]) {
Serial.println("CRC is not valid!");
return -2000;
}

if ( addr[0] != 0x10 && addr[0] != 0x28) {
Serial.print("Device is not recognized");
return -3000;
}

ds.reset();
ds.select(addr);
ds.write(0x44,1);

byte present = ds.reset();
ds.select(addr);
ds.write(0xBE);

for (int i = 0; i < 9; i++) {
data[i] = ds.read();
}

ds.reset_search();

byte MSB = data[1];
byte LSB = data[0];
float tempRead = ((MSB << 8) | LSB); //using two's compliment

  float celsius = (float)tempRead / 16.0;
  if(celsius > MaxTempThreshold || celsius < 0)
    digitalWrite(HEATER_RELAY_PIN, HIGH);
  else
    digitalWrite(HEATER_RELAY_PIN, LOW);
  
  return celsius;
}

void loadConfigFromMemory()
{
/*
  ShowDebugInformation = true;
  DisplayShortDate = false;
  DisplayTimeAs24HRClock = false;
  ShowTempInCelcius = true;
  StartWithFeedModeOn = true;;
  MaxTempThreshold = 25; // IN CELCIUS
  MinTempThreshold = 20; // IN CELCIUS
  FeedTimeInMin = 1;
  MinWaterLevel = 25;
  MaxWaterLevel = 23;
*/
}

void addButtons()
{
  feedButton = myButtons.addButton(5, 195, 60, 40, "W", BUTTON_SYMBOL);// | BUTTON_NO_BORDER);
  settingsButton = myButtons.addButton(129, 195, 60, 40, "w", BUTTON_SYMBOL);// | BUTTON_NO_BORDER);   
  debugButton = myButtons.addButton(250, 195, 60, 40, "y", BUTTON_SYMBOL);// | BUTTON_NO_BORDER); 
  myButtons.drawButtons();    
}

/* COUNTDOWN HELPER FUNCTIONS - SHOULD MOVE TO LIBRARY */

boolean CountDownTimer()
{
  static unsigned long duration = 1000000; // 1 second
  timeFlag = false;

  if (!timerStop && !timerPaused) // if not Stopped or Paused, run timer
  {
    // check the time difference and see if 1 second has elapsed
    if ((_micro = micros()) - time > duration )
    {
      Clock--;
      timeFlag = true;

      if (Clock == 0) // check to see if the clock is 0
        timerStop = true; // If so, stop the timer

     // check to see if micros() has rolled over, if not,
     // then increment "time" by duration
      _micro < time ? time = _micro : time += duration;
    }
  }
  return !timerStop; // return the state of the timer
}

void ResetTimer()
{
  SetTimer(R_clock);
  timerStop = false;
}

void StartTimer()
{
  Watch = micros(); // get the initial microseconds at the start of the timer
  timerStop = false;
  timerPaused = false;
}

void StopTimer()
{
  timerStop = true;
}

void StopTimerAt(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  if (TimeCheck(hours, minutes, seconds) )
    timerStop = true;
}

void PauseTimer()
{
  timerPaused = true;
}

void ResumeTimer() // You can resume the timer if you ever stop it.
{
  timerPaused = false;
}

void SetTimer(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  // This handles invalid time overflow ie 1(H), 0(M), 120(S) -> 1, 2, 0
  unsigned int _S = (seconds / 60), _M = (minutes / 60);
  if(_S) minutes += _S;
  if(_M) hours += _M;

  Clock = (hours * 3600) + (minutes * 60) + (seconds % 60);
  R_clock = Clock;
  timerStop = false;
}

void SetTimer(unsigned int seconds)
{
// StartTimer(seconds / 3600, (seconds / 3600) / 60, seconds % 60);
Clock = seconds;
R_clock = Clock;
timerStop = false;
}

int ShowHours()
{
  return Clock / 3600;
}

int ShowMinutes()
{
  return (Clock / 60) % 60;
}

int ShowSeconds()
{
  return Clock % 60;
}

unsigned long ShowMilliSeconds()
{
  return (_micro - Watch)/ 1000.0;
}

unsigned long ShowMicroSeconds()
{
  return _micro - Watch;
}

boolean TimeHasChanged()
{
  return timeFlag;
}

// output true if timer equals requested time
boolean TimeCheck(unsigned int hours, unsigned int minutes, unsigned int seconds)
{
  return (hours == ShowHours() && minutes == ShowMinutes() && seconds == ShowSeconds());
}
