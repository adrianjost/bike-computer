/**
Author: Adrian Jost
Version: 1.0.0
**/

// PIN DEFINITIONS

#define PIN_SENSOR 3
#define PIN_INPUT 1
#define PIN_SDA 2
#define PIN_SCK 0

#define SENSOR_SIGNAL HIGH
#define INPUT_PRESSED LOW

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
// https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp
// https://www.instructables.com/ESP8266-01-With-Multiple-I2C-Devices-Exploring-ESP/
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


WiFiManager wm;

FS* filesystem = &LittleFS;
WebSocketsServer webSocket = WebSocketsServer(80);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

void setup() {
  #ifdef DEBUG
    Serial.begin(DEBUG_SPEED);
    Serial.print("\n");
    Serial.setDebugOutput(true);
    Serial.println("STARTED IN DEBUG MODE");
  #endif

  #ifdef PIN_INPUT
    pinMode(PIN_INPUT, INPUT);
  #endif
  #ifdef PIN_SENSOR
    pinMode(PIN_SENSOR, INPUT);
  #endif

  setupFilesystem();

  #ifdef DEBUG
    Serial.println("setupFilesystem finished");
  #endif

  setupWifi();
  setupOTAUpdate();
  setupWebsocket();

  Wire.begin(PIN_SDA, PIN_SCK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.dim(true); // lower brightness

  loopStartTime = millis();
}

//*************************
// LOOP
//*************************

unsigned int sensorCount = 0;

void showSpeed(float speed){
  byte base = speed;

  display.clearDisplay();
  display.setTextSize(4);

  if(base >= 100){
    display.setCursor(24, 3);
    display.print(base);
  }else{
    byte rest = ((byte)(speed * 10)) - (base * 10);
    display.setCursor(base > 9 ? 0 : 24, 3);
    display.print(base);
    display.print(".");
    display.print(rest);
  }

  display.setCursor(102, 23);
  display.setTextSize(1); // 7x10 chars
  display.print("km/h");

  #ifdef PIN_INPUT
    if(digitalRead(PIN_INPUT) == INPUT_PRESSED){
      sensorCount = 0;
    }
  #endif

  #ifdef PIN_SENSOR
    if(digitalRead(PIN_SENSOR) == SENSOR_SIGNAL){
      sensorCount += 1;
    }
  #endif

  display.setCursor(100, 3);
  display.setTextSize(1);
  display.print(sensorCount);

  display.display();
}

void loop() {
  ArduinoOTA.handle(); // listen for OTA Updates
  webSocket.loop(); // listen for websocket events
  showSpeed((float)((millis() - loopStartTime) % 80000) / 750); // count up demo
}
