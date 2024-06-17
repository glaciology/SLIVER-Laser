// INCLUDE LIBRARIES
#include <SoftwareSerial.h> 
#include <MicroNMEA.h> 
#include <string.h> 
#include <SPI.h>
#include <WDT.h> 
#include "RTC.h"
#include <SD.h>

// DEFINE PMTK COMMANDS + LED PIN
#define PMTK_SET_NMEA_OUTPUT_RMCONLY  "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"
#define LEDpin 9

// SET BOOLEANS
bool fixFound = false;
bool wdtFlag = false; 

// SET GLOBAL VARIABLES
volatile int wdtCounterMax = 0; 
unsigned long prevMillis = 0; 
volatile int wdtCounter = 0; 
const int chipSelect = 10; 
static int buffIndex = 0;
char rangeTimeBuffer[25]; 
static char buffer[30];
char nmeabuffer[200];
uint lastChecked = 0; 
uint16_t* fileDate;
uint16_t* fileTime;
char filename[25];
char range[7]; 

// INITIALIZE OBJECTS
MicroNMEA nmea(nmeabuffer, sizeof(nmeabuffer));
SoftwareSerial gpsSerial(8, 7); 
APM3_RTC rtc; 
APM3_WDT wdt;
File logFile; 

void setup() {

  // INITIALIZE SERIAL PORTS
  Serial.begin(115200);
  gpsSerial.begin(9600); 
  delay(2000); 

  configureWdt(); 

  // SEND PMTK COMMANDS
  sendGPSCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  sendGPSCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  sendGPSCommand(PMTK_SET_BAUD_57600); 
  delay(100); 

  // CHANGE GPS BAUD
  gpsSerial.end(); 
  gpsSerial.begin(57600); 
  delay(500); 

  // LED SET UP
  pinMode(LEDpin, OUTPUT); 
  digitalWrite(LEDpin, LOW); 

  Serial.println("Beginning setup.");

  setupGPS_RTC_SD(); 

  // INITIALIZE LASER BAUD
  Serial1.begin(38400);
  delay(500); 

  Serial.println("Done with configuration.");
  blinkLED(10, 100); 

}

void loop() {

  petDog(); 

  if (rtc.seconds % 5 == 0 && rtc.seconds != lastChecked) {
    petDog();
    blinkLED(1, 100); 
    lastChecked = rtc.seconds;
  }

  while (Serial1.available() > 0) {
    // petDog(); 
    char c = Serial1.read(); 
    if (c == '\n' || buffIndex > 30) {
      buffer[buffIndex] = '\0';
      buffIndex = 0;
      if (buffer[8] == '.') {
        for (int i = 0; i < 6; i++) {
          range[i] = buffer[i + 6];
        }
        range[6] = '\0';
      } else {
        range[0] = '\0';
      }
    } else {
      buffer[buffIndex++] = c;
    }
  }

  rtc.getTime(); 

  sprintf(rangeTimeBuffer, "%02d %02d %02d %02d %02d %02d.%03d %s", rtc.dayOfMonth, rtc.month, rtc.year, rtc.hour, rtc.minute, rtc.seconds, rtc.hundredths, range); 

  Serial.println(rangeTimeBuffer); 
  logFile.println(rangeTimeBuffer);
  logFile.flush(); 
  delay(10); 

}

void setupGPS_RTC_SD() { 

  // INITIALIZE SD CARD
  if (!SD.begin(chipSelect)) {
    Serial.println("SD card failed.");
    while (1) {
      blinkLED(2, 250); 
      delay(2000); 
    }
  } else {
    Serial.println("SD card initialized.");
  }

  // GET VALID GPS FIX
  while (!fixFound) {
    petDog();
    digitalWrite(9, HIGH);
    while (gpsSerial.available()) {
      char ch = gpsSerial.read();
      nmea.process(ch);
      if (nmea.isValid()) {
        rtc.setTime(nmea.getHour(), nmea.getMinute(), nmea.getSecond(), nmea.getHundredths(), nmea.getDay(), nmea.getMonth(), nmea.getYear()-2000);
        Serial.println(nmea.getSentence());
        Serial.println("Valid fix!");
        fixFound = true;
        break;
      }
    }
    digitalWrite(9, LOW);
  } 

  // GET NAME FOR SD FILE
  for (uint8_t i = 0; i < 100; i++) {
    petDog();
    rtc.getTime();
    sprintf(filename, "%02d%02d%02d%02d.SCK", rtc.dayOfMonth, rtc.month, rtc.year, i);
    SdFile::dateTimeCallback(dateTime);
    if (!SD.exists(filename)) {
      break;
    }
  }

  // OPEN + SET UP SD FILE
  logFile = SD.open(filename, FILE_WRITE);
  delay(100);

  if (!logFile) {
    Serial.print(filename);
    Serial.println(" fail.");
    while (1) {
      blinkLED(2, 250);
      delay(1000);
    }
  } else {
    Serial.print(filename);
    Serial.println(" created.");
    logFile.print("# Laser ranges logged with the SICK logger 0.3.2 and AdaFruit Ultimate GPS+Logging Shield\n");
    logFile.println("# Day Month Year Hour Minute Second.Millisecond Range");
    const char* nmeaprint = nmea.getSentence();
    logFile.println(nmeaprint);
    logFile.flush();
  }

}

void blinkLED(byte ledFlashes, unsigned int ledDelay) {

  byte i = 0;
  while (i < ledFlashes * 2) {
    unsigned long currMillis = millis(); 
    if (currMillis - prevMillis >= ledDelay) {
    digitalWrite(LEDpin, !digitalRead(LEDpin));
    prevMillis = currMillis;
    i++;
    }
  }
  digitalWrite(LEDpin, LOW); 

}

void dateTime(uint16_t* date, uint16_t* time) {

  rtc.getTime();
  *date = FAT_DATE(rtc.year+2000, rtc.month, rtc.dayOfMonth);
  *time = FAT_TIME(rtc.hour, rtc.minute, rtc.seconds); 

}

void configureWdt() {

  wdt.configure(WDT_1HZ, 12, 24); 
  wdt.start(); 

}

void petDog() {

  wdt.restart();
  wdtFlag = false;
  wdtCounter = 0; 

}

void sendGPSCommand(const char* command) {

  gpsSerial.println(command);
  delay(100); 

}

extern "C" void am_watchdog_isr(void) {

  wdt.clear();
  logFile.println(wdtCounter); 

  if (wdtCounter < 5) {
    wdt.restart();
  }

  wdtFlag = true;
  wdtCounter++;

  if (wdtCounter > wdtCounterMax) {
    wdtCounterMax = wdtCounter; 
  }

}