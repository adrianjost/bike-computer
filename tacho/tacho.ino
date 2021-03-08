/**
Author: Adrian Jost
Version: 1.0.0
**/

// DEVICE CONFIGURATION

#define PIN_SENSOR 3
#define PIN_BUTTON 1
#define PIN_SDA 2
#define PIN_SCK 0

// HIGH, LOW
#define SENSOR_TRIGGERED LOW
#define BUTTON_PRESSED LOW

// only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
#define WAKEUP_SENSOR GPIO_PIN_INTR_LOLEVEL
#define WAKEUP_BUTTON GPIO_PIN_INTR_LOLEVEL

// CHANGE, RISING, FALLING
#define INTERRUPT_SENSOR RISING
#define INTERRUPT_BUTTON RISING

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     1

#define MENU_ITEMS 2
#define LOW_POWER_DELAY 1000

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

// OTA Updates
#include <ArduinoOTA.h>

// Display & RTC
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

// Low Power Sleep States
extern "C" {
  #include "gpio.h"
}
extern "C" {
  #include "user_interface.h"
}

WiFiManager wm;
FS* filesystem = &LittleFS;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

// JSON sizes https://arduinojson.org/v6/assistant/
// { "hostname": "abcdef" }
const size_t maxWifiConfigSize = JSON_OBJECT_SIZE(2) + 80;

//*************************
// GLOBAL STATE
//*************************

char hostname[32] = "A CHIP";
char wifiPassword[32] = "";

volatile unsigned int sensorCount = 0;
volatile unsigned long lastSensorInterrupt = 0;
volatile byte menuItem = 0;
volatile unsigned long lastButtonInterrupt = 0;

/**********************************
 UTILS
**********************************/

void timedLightSleepCallback() {
  // do nothing
}
void timedLightSleep(unsigned int sleepMs){
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  extern os_timer_t *timer_list;
  timer_list = nullptr;  // stop (but don't disable) the 4 OS timers
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  #ifdef PIN_BUTTON
    gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_BUTTON), WAKEUP_BUTTON); // (optional)
  #endif
  #ifdef PIN_SENSOR
    gpio_pin_wakeup_enable(PIN_SENSOR, WAKEUP_SENSOR); // (optional)
  #endif
  wifi_fpm_set_wakeup_cb(timedLightSleepCallback); // set wakeup callback
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_open();
  wifi_fpm_do_sleep(sleepMs * 1000);  // Sleep range = 10000 ~ 268,435,454 uS (0xFFFFFFE, 2^28-1)
  delay(sleepMs + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void sleep(unsigned int duration){
  unsigned long now = millis();
  if ((now - lastSensorInterrupt < LOW_POWER_DELAY) || (now - lastButtonInterrupt < LOW_POWER_DELAY)) {
    // we are active, keep everything alive
    if(duration < LOW_POWER_DELAY){
      delay(duration);
    }else{
      delay(LOW_POWER_DELAY);
    }
  }else{
    timedLightSleep(duration);
  }
}

//*************************
// INPUTS
//*************************

void ICACHE_RAM_ATTR handleSensorInterrupt();
void handleSensorInterrupt() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastSensorInterrupt < 500) {
    return;
  }
  sensorCount++;
  lastSensorInterrupt = interruptTime;
}

void ICACHE_RAM_ATTR handleButtonInterrupt();
void handleButtonInterrupt() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastButtonInterrupt < 200) {
    return;
  }
  menuItem = (menuItem + 1) % MENU_ITEMS;
  lastButtonInterrupt = interruptTime;
}

//*************************
// SCREENS
//*************************

void drawMenuPosition(byte position) {
  byte width = SCREEN_WIDTH / MENU_ITEMS;
  display.drawFastHLine(width * position, SCREEN_HEIGHT - 1, width, WHITE);
}

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

  display.setCursor(100, 0);
  display.setTextSize(1);
  display.print(sensorCount);

  drawMenuPosition(0);

  display.display();
}

void showDateTime() {
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
// SETUP
//*************************

unsigned int loopStartTime;
bool wifiActive = false;

void setup() {
  #ifdef DEBUG
    Serial.begin(DEBUG_SPEED);
    Serial.print("\n");
    Serial.setDebugOutput(true);
    Serial.println("STARTED IN DEBUG MODE");
  #endif

  #ifdef PIN_BUTTON
    pinMode(PIN_BUTTON, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), handleButtonInterrupt, INTERRUPT_BUTTON);
  #endif

  #ifdef PIN_SENSOR
    pinMode(PIN_SENSOR, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_SENSOR), handleSensorInterrupt, INTERRUPT_SENSOR);
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
  #ifdef PIN_SENSOR
    // WIFI stuff disabled during interrupt & sleep experiments
    // need to be manually reenabled when woken up from light-sleep
    if(false && digitalRead(PIN_SENSOR) != SENSOR_TRIGGERED){
      wifiActive = true;
      display.print("WiFi...");
      display.display();
      setupWifi();
      setupOTAUpdate();
      display.clearDisplay();
      display.setCursor(0, 14);
      display.print("WiFi UP");
      display.display();
    }else{
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      wifiActive = false;
      display.print("WiFi DOWN");
      display.display();
    }
  #else
    WiFi.mode(WIFI_OFF);
    wifiActive = false;
    display.setTextSize(1);
    display.print("NO WIFI MODE");
    display.display();
  #endif

  delay(1000);

  loopStartTime = millis();
}

//*************************
// LOOP
//*************************


// unsigned long lastWakeUp = 0;
// unsigned long lastHandledSensorInterrupt = 0;

void loop() {
  switch (menuItem) {
    case 0: {
      DateTime now = rtc.now();
      showSpeed((float)now.second());
      // showSpeed((float)((millis() - loopStartTime) % 110000) / 1000);
      sleep(1000);
      break;
    }
    case 1: {
      showDateTime();
      // TODO somehow the screen change is only rendered after the timeout, but the interrupt is working, so the CPU must be waked up.
      sleep(5000); // TODO extend to 60s and remove seconds from clock view
      break;
    }
    default:
      break;
  }
}
