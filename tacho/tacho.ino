/**
Author: Adrian Jost
Version: 1.0.0
**/

// PIN DEFINITIONS

#define PIN_SENSOR 3
#define PIN_BUTTON 1
#define PIN_SDA 2
#define PIN_SCK 0

#define SENSOR_SIGNAL HIGH
#define BUTTON_PRESSED LOW

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     1

// config storage
#define PATH_CONFIG_WIFI "/config.json"

// WiFiManager
// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiManager.h>        // 1.0.0 - tzapu,tablatronix (GitHub Develop Branch c9665ad)
#include <FS.h>
#include <LittleFS.h>           // requires esp8266-core >2.6.3

// Website Communication
#include <ESP8266WiFi.h>        // 1.0.0 - Ivan Grokhotkov
#include <ArduinoJson.h>        // 6.16.1 - Benoit Blanchon
#include <WebSocketsServer.h>   // 2.1.4 Markus Sattler

// OTA Updates
#include <ArduinoOTA.h>

// Display
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

WiFiManager wm;

FS* filesystem = &LittleFS;
WebSocketsServer webSocket = WebSocketsServer(80);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

// JSON sizes https://arduinojson.org/v6/assistant/
// { "hostname": "abcdef" }
const size_t maxWifiConfigSize = JSON_OBJECT_SIZE(2) + 80;

//*************************
// global State
//*************************

char hostname[32] = "A CHIP";
char wifiPassword[32] = "";

/**********************************
 INIT
**********************************/


//*************************
// config manager
//*************************

bool shouldSaveConfig = false;
WiFiManager *globalWiFiManager;

void saveConfigCallback () {
  #ifdef DEBUG
    Serial.println("shouldSaveConfig");
  #endif
  shouldSaveConfig = true;
  globalWiFiManager->stopConfigPortal();
}

void configModeCallback (WiFiManager *myWiFiManager) {
  globalWiFiManager = myWiFiManager;
  #ifdef DEBUG
    Serial.println("start config portal");
  #endif
}

void setupFilesystem() {
  #ifdef DEBUG
    Serial.println("setupFilesystem");
  #endif

  // initial values
  ("SmartLight-" + String(ESP.getChipId(), HEX)).toCharArray(hostname, 32);

  #ifdef DEBUG
    Serial.print("hostname: ");
    Serial.println(hostname);
  #endif

  #ifdef DEBUG
    Serial.println("exec filesystem->begin()");
  #endif
  filesystem->begin();
  #ifdef DEBUG
    Serial.println("filesystem->begin() executed");
  #endif

  if(!filesystem->exists(PATH_CONFIG_WIFI)) {
    #ifdef DEBUG
      Serial.println("config file doesn't exist");
    #endif
    return;
  }
  #ifdef DEBUG
    Serial.println("configfile exists");
  #endif

  //file exists, reading and loading
  File configFile = filesystem->open(PATH_CONFIG_WIFI, "r");
  if(!configFile) { return; }
  #ifdef DEBUG
    Serial.println("configfile read");
  #endif

  size_t size = configFile.size();
  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  DynamicJsonDocument doc(maxWifiConfigSize);
  auto error = deserializeJson(doc, buf.get());
  if(error){ return; }
  configFile.close();

  #ifdef DEBUG
    Serial.println("configfile serialized");
  #endif

  // copy from config to variable
  if(doc.containsKey("hostname")){
    #ifdef DEBUG
      Serial.println("hostname key read");
    #endif
    strcpy(hostname, doc["hostname"]);
    #ifdef DEBUG
      Serial.println("hostname updated");
    #endif
  }
}

bool shouldEnterSetup(){
  #ifndef PIN_RESET
    return false;
  #else
    pinMode(PIN_RESET, INPUT);
    byte clickThreshould = 5;
    int timeSlot = 5000;
    byte readingsPerSecond = 10;
    byte click_count = 0;


    for(int i=0; i < (timeSlot / readingsPerSecond / 10); i++){
      byte buttonState = digitalRead(PIN_RESET);
      if(buttonState == LOW){
        click_count++;
        if(click_count >= clickThreshould){
          pinMode(PIN_RESET, OUTPUT);
          digitalWrite(PIN_RESET, HIGH);
          return true;
        }
      } else {
        click_count = 0;
      }
      delay(1000 / readingsPerSecond);
    }
    return false;
  #endif
}

void setupWifi(){
  WiFi.mode(WIFI_STA);

  wm.setDebugOutput(false);
  // close setup after 5min
  wm.setTimeout(300);
  // set dark theme
  wm.setClass("invert");

  wm.setSaveParamsCallback(saveConfigCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setAPCallback(configModeCallback);

  wm.setHostname(hostname);

  WiFiManagerParameter setting_hostname("hostname", "Devicename: (e.g. <code>smartlight-kitchen</code>)", hostname, 32);
  wm.addParameter(&setting_hostname);

  bool forceSetup = false; // TODO: shouldEnterSetup();
  bool setup = forceSetup
    ? wm.startConfigPortal("SmartLight Setup", "LightItUp")
    : wm.autoConnect("SmartLight Setup", "LightItUp");

  if (shouldSaveConfig) {
    #ifdef DEBUG
      Serial.println("write config to filesystem");
    #endif
    DynamicJsonDocument doc(maxWifiConfigSize);

    doc["hostname"] = setting_hostname.getValue();

    File configFile = filesystem->open(PATH_CONFIG_WIFI, "w");
    serializeJson(doc, configFile);
    configFile.close();

    #ifdef DEBUG
      Serial.println("config written to filesystem");
    #endif

    ESP.restart();
  }

  if(!setup){
    // shut down till the next reboot
    // ESP.deepSleep(86400000000); // 1 Day
    ESP.deepSleep(300000000); // 5 Minutes
    ESP.restart();
  }

  if(forceSetup){
    ESP.restart();
  }

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
}

void setupOTAUpdate(){
  wm.getWiFiPass().toCharArray(wifiPassword, 32);
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.setPassword(wifiPassword);

  #ifdef DEBUG
    ArduinoOTA.onStart([]() {
      Serial.println("Start");
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nEnd");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  #endif
  ArduinoOTA.begin();
  #ifdef DEBUG
    Serial.println("OTA ready");
  #endif
}


void setupDisplay(){
  Wire.begin(PIN_SDA, PIN_SCK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.dim(true); // lower brightness
}

void setupRTC(){
  if (! rtc.begin()) {
    // Couldn't find RTC
    display.clearDisplay();
    display.setCursor(5, 12);
    display.setTextSize(3);
    display.print("NO RTC");
    display.display();
    delay(1000);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
}

//*************************
// websocket communication
//*************************

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_TEXT:{
      webSocket.sendTXT(num, payload, length);
      break;
    }
  }
}

void setupWebsocket(){
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

//*************************
// SETUP
//*************************

unsigned int loopStartTime;
byte wifiActive = false;

void setup() {
  #ifdef DEBUG
    Serial.begin(DEBUG_SPEED);
    Serial.print("\n");
    Serial.setDebugOutput(true);
    Serial.println("STARTED IN DEBUG MODE");
  #endif

  #ifdef PIN_BUTTON
    pinMode(PIN_BUTTON, INPUT);
  #endif
  #ifdef PIN_SENSOR
    pinMode(PIN_SENSOR, INPUT);
  #endif

  setupDisplay();

  setupFilesystem();

  #ifdef DEBUG
    Serial.println("setupFilesystem finished");
  #endif

  setupRTC();

  display.clearDisplay();
  display.setCursor(0, 14);
  display.setTextSize(2);
  if(digitalRead(PIN_SENSOR) != SENSOR_SIGNAL){
    wifiActive = true;
    display.print("WiFi...");
    display.display();
    setupWifi();
    setupOTAUpdate();
    setupWebsocket();
    display.clearDisplay();
    display.setCursor(0, 14);
    display.print("WiFi UP");
    display.display();
  }else{
    WiFi.mode(WIFI_OFF);
    wifiActive = false;
    display.print("WiFi DOWN");
    display.display();
  }

  delay(1000);

  loopStartTime = millis();
}

//*************************
// LOOP
//*************************

#define MENU_ITEMS 2
void drawMenuPosition(byte position) {
  byte width = SCREEN_WIDTH / MENU_ITEMS;
  display.drawFastHLine(width * position, SCREEN_HEIGHT - 1, width, WHITE);
}

unsigned int sensorCount = 0;

void showSpeed(float speed){
  byte base = speed;

  display.clearDisplay();
  display.setTextSize(4);

  if(base >= 100){
    display.setCursor(24, 0);
    display.print(base);
  }else{
    byte rest = ((byte)(speed * 10)) - (base * 10);
    display.setCursor(base > 9 ? 0 : 24, 0);
    display.print(base);
    display.print(".");
    display.print(rest);
  }

  display.setCursor(102, 20);
  display.setTextSize(1);
  display.print("km/h");

  #ifdef PIN_BUTTON
    if(digitalRead(PIN_BUTTON) == BUTTON_PRESSED){
      sensorCount = 0;
    }
  #endif

  #ifdef PIN_SENSOR
    if(digitalRead(PIN_SENSOR) == SENSOR_SIGNAL){
      sensorCount += 1;
    }
  #endif

  display.setCursor(100, 0);
  display.setTextSize(1);
  display.print(sensorCount);

  drawMenuPosition(0);

  display.display();
}

unsigned int lastTimeUpdate = 0;
void showDateTime() {
  if(millis() - lastTimeUpdate < 1000){
    // updating the screen every second is enough
    return;
  }

  lastTimeUpdate = millis();
  DateTime now = rtc.now();

  display.clearDisplay();
  display.setCursor(2, 5);
  display.setTextSize(3);

  byte hour = now.hour();
  if(hour < 10){
    display.print("0");
  }
  display.print(hour);

  display.print(":");

  byte minute = now.minute();
  if(minute < 10){
    display.print("0");
  }
  display.print(minute);

  display.setTextSize(2);
  display.setCursor(102, 12);
  byte second = now.second();
  if(second < 10){
    display.print("0");
  }
  display.print(second);

  drawMenuPosition(1);

  display.display();
}

byte menuItem = 0;

void loop() {
  if(wifiActive){
    ArduinoOTA.handle(); // listen for OTA Updates
    webSocket.loop(); // listen for websocket events
  }

  if(digitalRead(PIN_BUTTON) == BUTTON_PRESSED){
    menuItem = (menuItem + 1) % MENU_ITEMS;
    while(digitalRead(PIN_BUTTON) == BUTTON_PRESSED){
      // do nothing
    }
  }

  switch (menuItem) {
    case 0:
      showSpeed((float)((millis() - loopStartTime) % 80000) / 750);
      break;
    case 1:
      showDateTime();
      break;
    default:
      break;
  }
  
}
