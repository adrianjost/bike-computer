/**
Author: Adrian Jost
Version: 1.0.0
**/

// DEVICE CONFIGURATION

#define PIN_BUTTON 3
#define PIN_WAKEUP_INTERVAL 13
#define PIN_SDA 2
#define PIN_SCK 0

// Attiny
#define ATTINY_ADDRESS 0x4
// TODO [#8]: make WHEEL_CIRCUMFERENCE adjustable by user - step 1: use a variable
#define WHEEL_CIRCUMFERENCE 2000

// HIGH, LOW
#define BUTTON_PRESSED LOW

// only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
#define WAKEUP_BUTTON GPIO_PIN_INTR_LOLEVEL
#define WAKEUP_INTERVAL GPIO_PIN_INTR_HILEVEL

// CHANGE, RISING, FALLING
#define INTERRUPT_BUTTON RISING

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 1

#define MENU_ITEMS 2
#define LOW_POWER_DELAY 1000

// config storage
#define PATH_CONFIG_WIFI "/config.json"

// WiFiManager
// https://github.com/tzapu/WiFiManager WiFi Configuration Magic
// #include <FS.h>
// #include <LittleFS.h>     // requires esp8266-core >2.6.3
// #include <WiFiManager.h>  // 1.0.0 - tzapu,tablatronix (GitHub Develop Branch c9665ad)

// Website Communication
// #include <ArduinoJson.h>  // 6.16.1 - Benoit Blanchon
#include <ESP8266WiFi.h>  // 1.0.0 - Ivan Grokhotkov

// OTA Updates
// #include <ArduinoOTA.h>

// Display & RTC
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib
#include "RTClib.h"

// Low Power Sleep States
extern "C" {
#include "gpio.h"
}
extern "C" {
#include "user_interface.h"
}

// WiFiManager wm;
// FS *filesystem = &LittleFS;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

// JSON sizes https://arduinojson.org/v6/assistant/
// { "hostname": "abcdef" }
// const size_t maxWifiConfigSize = JSON_OBJECT_SIZE(2) + 80;

//*************************
// GLOBAL STATE
//*************************

// char hostname[32] = "A CHIP";
// char wifiPassword[32] = "";

float speed = 0.0;
unsigned int tripRotations = 0;
unsigned long tripSeconds = 0;
byte batteryLevel = 0;

volatile byte menuItem = 0;
volatile unsigned long lastButtonInterrupt = 0;

/**********************************
 UTILS
**********************************/

void timedLightSleepCallback() {
  // do nothing
}
void timedLightSleep(unsigned int sleepMs) {
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  extern os_timer_t *timer_list;
  timer_list = nullptr;  // stop (but don't disable) the 4 OS timers
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
#ifdef PIN_BUTTON
  gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_BUTTON), WAKEUP_BUTTON);  // (optional)
#endif
  wifi_fpm_set_wakeup_cb(timedLightSleepCallback);  // set wakeup callback
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_open();
  wifi_fpm_do_sleep(sleepMs * 1000);  // Sleep range = 10000 ~ 268,435,454 uS (0xFFFFFFE, 2^28-1)
  delay(sleepMs + 1);                 // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void forcedLightSleep() {
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
#ifdef PIN_BUTTON
  gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_BUTTON), WAKEUP_BUTTON);
#endif
#ifdef PIN_WAKEUP_INTERVAL
  gpio_pin_wakeup_enable(GPIO_ID_PIN(PIN_WAKEUP_INTERVAL), WAKEUP_INTERVAL);
#endif
  // wifi_fpm_set_wakeup_cb(wakeupCallback); // Set wakeup callback (optional)
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);  // only 0xFFFFFFF, any other value and it won't disconnect the RTC timer
  delay(10);                     // it goes to sleep during this delay() and waits for an interrupt
}

void sleep() {
  forcedLightSleep();
}

//*************************
// INPUTS
//*************************

void ICACHE_RAM_ATTR handleButtonInterrupt();
void handleButtonInterrupt() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastButtonInterrupt < 200) {
    return;
  }
  menuItem = (menuItem + 1) % MENU_ITEMS;
  lastButtonInterrupt = interruptTime;
}

// float speed = 0.0;
// unsigned int tripRotations = 0;

bool stopped = true;
unsigned long lastTimeIncrease = 0;
void fetchData() {
  Wire.requestFrom(ATTINY_ADDRESS, 2);
  byte rPer20s = 0;
  byte batteryAndRotationCount = 0;
  byte read = 0;
  while (Wire.available()) {
    if (read == 0) {
      rPer20s = Wire.read();
    } else if (read == 1) {
      batteryAndRotationCount = Wire.read();
    } else {
      Wire.read();
    }
    read++;
  }

  speed = (float)(rPer20s * WHEEL_CIRCUMFERENCE * 3.6) / 20000.0;
  batteryLevel = batteryAndRotationCount & B00001111;
  tripRotations += ((batteryAndRotationCount & B11110000) >> 4);

  unsigned long now = rtc.now().secondstime();
  if (stopped == false) {
    tripSeconds += now - lastTimeIncrease;
  }
  if (rPer20s == 0) {
    stopped = true;
  } else {
    lastTimeIncrease = now;
    stopped = false;
  }
}

//*************************
// SCREENS
//*************************

// ICON HELPERS
void drawIconClock(byte x, byte y) {
  display.drawFastVLine(x + 0, y + 4, 6, WHITE);
  display.drawFastVLine(x + 1, y + 4, 6, WHITE);

  display.drawFastVLine(x + 2, y + 2, 2, WHITE);
  display.drawFastVLine(x + 3, y + 2, 2, WHITE);
  display.drawFastVLine(x + 2, y + 10, 2, WHITE);
  display.drawFastVLine(x + 3, y + 10, 2, WHITE);

  display.drawFastVLine(x + 4, y + 0, 2, WHITE);
  display.drawFastVLine(x + 5, y + 0, 2, WHITE);
  display.drawFastVLine(x + 4, y + 12, 2, WHITE);
  display.drawFastVLine(x + 5, y + 12, 2, WHITE);

  display.drawFastVLine(x + 6, y + 0, 8, WHITE);
  display.drawFastVLine(x + 7, y + 0, 8, WHITE);
  display.drawFastVLine(x + 6, y + 12, 2, WHITE);
  display.drawFastVLine(x + 7, y + 12, 2, WHITE);

  display.drawFastVLine(x + 8, y + 0, 2, WHITE);
  display.drawFastVLine(x + 9, y + 0, 2, WHITE);
  display.drawFastVLine(x + 8, y + 6, 2, WHITE);
  display.drawFastVLine(x + 9, y + 6, 2, WHITE);
  display.drawFastVLine(x + 8, y + 12, 2, WHITE);
  display.drawFastVLine(x + 9, y + 12, 2, WHITE);

  display.drawFastVLine(x + 10, y + 2, 2, WHITE);
  display.drawFastVLine(x + 11, y + 2, 2, WHITE);
  display.drawFastVLine(x + 10, y + 10, 2, WHITE);
  display.drawFastVLine(x + 11, y + 10, 2, WHITE);

  display.drawFastVLine(x + 12, y + 4, 6, WHITE);
  display.drawFastVLine(x + 13, y + 4, 6, WHITE);
}
void drawIconStopwatch(byte x, byte y) {
  drawIconClock(x, y);
  display.drawFastVLine(x + 0, y + 0, 2, WHITE);
  display.drawFastVLine(x + 1, y + 0, 2, WHITE);
  display.drawFastVLine(x + 12, y + 0, 2, WHITE);
  display.drawFastVLine(x + 13, y + 0, 2, WHITE);
}
void drawIconDistance(byte x, byte y) {
  display.drawPixel(x + 3, y + 0, WHITE);
  display.drawPixel(x + 10, y + 0, WHITE);

  display.drawFastHLine(x + 2, y + 1, 2, WHITE);
  display.drawFastHLine(x + 10, y + 1, 2, WHITE);

  display.drawFastHLine(x + 1, y + 2, 3, WHITE);
  display.drawFastHLine(x + 10, y + 2, 3, WHITE);

  display.drawFastHLine(x + 0, y + 3, 14, WHITE);
  display.drawFastHLine(x + 0, y + 4, 14, WHITE);

  display.drawFastHLine(x + 1, y + 5, 3, WHITE);
  display.drawFastHLine(x + 10, y + 5, 3, WHITE);

  display.drawFastHLine(x + 2, y + 6, 2, WHITE);
  display.drawFastHLine(x + 10, y + 6, 2, WHITE);

  display.drawPixel(x + 3, y + 7, WHITE);
  display.drawPixel(x + 10, y + 7, WHITE);
}
void drawIconArrowUp(byte x, byte y) {
  display.drawFastVLine(x + 0, y + 6, 2, WHITE);
  display.drawFastVLine(x + 1, y + 6, 2, WHITE);

  display.drawFastVLine(x + 2, y + 4, 2, WHITE);
  display.drawFastVLine(x + 3, y + 4, 2, WHITE);

  display.drawFastVLine(x + 4, y + 2, 2, WHITE);
  display.drawFastVLine(x + 5, y + 2, 2, WHITE);

  display.drawFastVLine(x + 6, y + 0, 2, WHITE);
  display.drawFastVLine(x + 7, y + 0, 2, WHITE);

  display.drawFastVLine(x + 8, y + 2, 2, WHITE);
  display.drawFastVLine(x + 9, y + 2, 2, WHITE);

  display.drawFastVLine(x + 10, y + 4, 2, WHITE);
  display.drawFastVLine(x + 11, y + 4, 2, WHITE);

  display.drawFastVLine(x + 12, y + 6, 2, WHITE);
  display.drawFastVLine(x + 13, y + 6, 2, WHITE);
}
void drawIconArrowDown(byte x, byte y) {
  display.drawFastVLine(x + 0, y + 0, 2, WHITE);
  display.drawFastVLine(x + 1, y + 0, 2, WHITE);

  display.drawFastVLine(x + 2, y + 2, 2, WHITE);
  display.drawFastVLine(x + 3, y + 2, 2, WHITE);

  display.drawFastVLine(x + 4, y + 4, 2, WHITE);
  display.drawFastVLine(x + 5, y + 4, 2, WHITE);

  display.drawFastVLine(x + 6, y + 6, 2, WHITE);
  display.drawFastVLine(x + 7, y + 6, 2, WHITE);

  display.drawFastVLine(x + 8, y + 4, 2, WHITE);
  display.drawFastVLine(x + 9, y + 4, 2, WHITE);

  display.drawFastVLine(x + 10, y + 2, 2, WHITE);
  display.drawFastVLine(x + 11, y + 2, 2, WHITE);

  display.drawFastVLine(x + 12, y + 0, 2, WHITE);
  display.drawFastVLine(x + 13, y + 0, 2, WHITE);
}

void drawMenuPosition(byte position) {
  byte width = SCREEN_WIDTH / MENU_ITEMS;
  display.drawFastHLine(width * position, SCREEN_HEIGHT - 1, width, WHITE);
}

void showSpeed() {
  byte base = speed;

  display.setTextSize(4);

  if (base >= 100) {
    display.setCursor(24, 0);
    display.print(base);
  } else {
    byte rest = ((byte)(speed * 10)) - (base * 10);
    display.setCursor(base > 9 ? 0 : 24, 0);
    display.print(base);
    display.print(".");
    display.print(rest);
  }

  display.setCursor(102, 20);
  display.setTextSize(1);
  display.print("km/h");

  // draw arrow to compare current speed to avg speed from left to right
  // TODO [#11]: draw correct arrow relative to avg speed
  // drawIconArrowUp(106, 6);
  drawIconArrowDown(106, 6);
}

void showCurrentTrip() {
  display.setCursor(0, 30);
  display.setTextSize(2);

  drawIconDistance(0, 33);

  unsigned long tripM = (tripRotations * WHEEL_CIRCUMFERENCE) / 1000;
  unsigned long tripKm = tripM / 1000;
  // TODO [#12]: digit extraction does not work if the first deximal place is a 0
  // it get's lost because 04 is converted to the number 4.
  // example: 1004m => 1.004km but 1.4km is printed
  // that's the case, because 1004 - (1 * 1000) = 4 not 004 like it should be.
  display.print("  ");
  if (tripKm < 1) {
    if (tripM < 10) {
      display.print("     ");
    } else if (tripM < 100) {
      display.print("    ");
    } else if (tripM < 1000) {
      display.print("   ");
    } else {
      display.print("  ");
    }
    display.print(tripM);
    display.println("m");
  } else if (tripKm < 100) {  // 3 decimal places
    if (tripKm < 10) {
      display.print(" ");
    }
    display.print(tripKm);
    display.print(".");
    display.print(tripM - (tripKm * 1000));
    display.println("km");
  } else if (tripKm < 1000) {  // 2 decimal places
    display.print(tripKm);
    display.print(".");
    display.print((tripM / 10) - (tripKm * 100));
    display.println("km");
  } else if (tripKm < 10000) {  // 1 decimal place
    display.print(tripKm);
    display.print(".");
    display.print((tripM / 100) - (tripKm * 10));
    display.println("km");
  } else {  // no decimals
    if (tripKm < 1000000) {
      display.print(" ");
    }
    display.print(tripKm);
    display.println("km");
  }

  // draw stopwatch from left to right at 2x scale
  drawIconStopwatch(0, 46);
  unsigned long tripMinutes = tripSeconds / 60;
  unsigned long tripHours = tripMinutes / 60;
  byte h = tripHours % 60;
  display.print("  ");
  if (h <= 9) {
    display.print("0");
  }
  display.print(h);
  display.print(":");
  byte m = tripMinutes % 60;
  if (m <= 9) {
    display.print("0");
  }
  display.print(m);
  display.print(":");
  byte s = tripSeconds % 60;
  if (s <= 9) {
    display.print("0");
  }
  display.println(s);

  // TODO [#4]: simplify mathematics
  // TODO [#5]: extract avgSpeed calculation into global scope to be reused on current speed screen
  // float avgSpeed = (float)(((tripRotations * WHEEL_CIRCUMFERENCE) / tripSeconds) / 1000.0) * 3.6;
  // byte avgSpeedBase = (byte)avgSpeed;
  // display.print("v: ");
  // display.print(avgSpeedBase);
  // display.print(".");
  // display.print((byte)((avgSpeed * 10) - (avgSpeedBase * 10)));
  // display.print(" km/h");
}

void showDateTime() {
  DateTime now = rtc.now();

  drawIconClock(2, 40);

  display.setCursor(20, 37);
  display.setTextSize(3);

  byte hour = now.hour();
  if (hour < 10) {
    display.print("0");
  }
  display.print(hour);

  display.print(":");

  byte minute = now.minute();
  if (minute < 10) {
    display.print("0");
  }
  display.print(minute);

  // display.setTextSize(2);
  // display.setCursor(102, 42);
  // byte second = now.second();
  // if (second < 10) {
  //   display.print("0");
  // }
  // display.print(second);
}

void updateScreen() {
  display.clearDisplay();
  showSpeed();
  switch (menuItem) {
    case 0: {
      showCurrentTrip();
      break;
    }
    case 1: {
      showDateTime();
      break;
    }
    default:
      break;
  }
  drawMenuPosition(menuItem);
  display.display();
}

/**********************************
 INIT
**********************************/

//*************************
// config manager
//*************************

// bool shouldSaveConfig = false;
// WiFiManager *globalWiFiManager;

// void saveConfigCallback() {
// #ifdef DEBUG
//   Serial.println("shouldSaveConfig");
// #endif
//   shouldSaveConfig = true;
//   globalWiFiManager->stopConfigPortal();
// }

// void configModeCallback(WiFiManager *myWiFiManager) {
//   globalWiFiManager = myWiFiManager;
// #ifdef DEBUG
//   Serial.println("start config portal");
// #endif
// }

// void setupFilesystem() {
// #ifdef DEBUG
//   Serial.println("setupFilesystem");
// #endif

//   // initial values
//   ("SmartLight-" + String(ESP.getChipId(), HEX)).toCharArray(hostname, 32);

// #ifdef DEBUG
//   Serial.print("hostname: ");
//   Serial.println(hostname);
// #endif

// #ifdef DEBUG
//   Serial.println("exec filesystem->begin()");
// #endif
//   filesystem->begin();
// #ifdef DEBUG
//   Serial.println("filesystem->begin() executed");
// #endif

//   if (!filesystem->exists(PATH_CONFIG_WIFI)) {
// #ifdef DEBUG
//     Serial.println("config file doesn't exist");
// #endif
//     return;
//   }
// #ifdef DEBUG
//   Serial.println("configfile exists");
// #endif

//   //file exists, reading and loading
//   File configFile = filesystem->open(PATH_CONFIG_WIFI, "r");
//   if (!configFile) {
//     return;
//   }
// #ifdef DEBUG
//   Serial.println("configfile read");
// #endif

//   size_t size = configFile.size();
//   // Allocate a buffer to store contents of the file.
//   std::unique_ptr<char[]> buf(new char[size]);
//   configFile.readBytes(buf.get(), size);
//   DynamicJsonDocument doc(maxWifiConfigSize);
//   auto error = deserializeJson(doc, buf.get());
//   if (error) {
//     return;
//   }
//   configFile.close();

// #ifdef DEBUG
//   Serial.println("configfile serialized");
// #endif

//   // copy from config to variable
//   if (doc.containsKey("hostname")) {
// #ifdef DEBUG
//     Serial.println("hostname key read");
// #endif
//     strcpy(hostname, doc["hostname"]);
// #ifdef DEBUG
//     Serial.println("hostname updated");
// #endif
//   }
// }

// void setupWifi() {
//   WiFi.mode(WIFI_STA);

//   wm.setDebugOutput(false);
//   // close setup after 5min
//   wm.setTimeout(300);
//   // set dark theme
//   wm.setClass("invert");

//   wm.setSaveParamsCallback(saveConfigCallback);
//   wm.setSaveConfigCallback(saveConfigCallback);
//   wm.setAPCallback(configModeCallback);

//   wm.setHostname(hostname);

//   WiFiManagerParameter setting_hostname("hostname", "Devicename: (e.g. <code>smartlight-kitchen</code>)", hostname, 32);
//   wm.addParameter(&setting_hostname);

//   bool forceSetup = false;  // TODO: shouldEnterSetup();
//   bool setup = forceSetup
//                    ? wm.startConfigPortal("SmartLight Setup", "LightItUp")
//                    : wm.autoConnect("SmartLight Setup", "LightItUp");

//   if (shouldSaveConfig) {
// #ifdef DEBUG
//     Serial.println("write config to filesystem");
// #endif
//     DynamicJsonDocument doc(maxWifiConfigSize);

//     doc["hostname"] = setting_hostname.getValue();

//     File configFile = filesystem->open(PATH_CONFIG_WIFI, "w");
//     serializeJson(doc, configFile);
//     configFile.close();

// #ifdef DEBUG
//     Serial.println("config written to filesystem");
// #endif

//     ESP.restart();
//   }

//   if (!setup) {
//     // shut down till the next reboot
//     // ESP.deepSleep(86400000000); // 1 Day
//     ESP.deepSleep(300000000);  // 5 Minutes
//     ESP.restart();
//   }

//   if (forceSetup) {
//     ESP.restart();
//   }

//   WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
// }

// void setupOTAUpdate() {
//   wm.getWiFiPass().toCharArray(wifiPassword, 32);
//   ArduinoOTA.setHostname(hostname);
//   ArduinoOTA.setPassword(wifiPassword);

// #ifdef DEBUG
//   ArduinoOTA.onStart([]() {
//     Serial.println("Start");
//   });
//   ArduinoOTA.onEnd([]() {
//     Serial.println("\nEnd");
//   });
//   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//   });
//   ArduinoOTA.onError([](ota_error_t error) {
//     Serial.printf("Error[%u]: ", error);
//     if (error == OTA_AUTH_ERROR)
//       Serial.println("Auth Failed");
//     else if (error == OTA_BEGIN_ERROR)
//       Serial.println("Begin Failed");
//     else if (error == OTA_CONNECT_ERROR)
//       Serial.println("Connect Failed");
//     else if (error == OTA_RECEIVE_ERROR)
//       Serial.println("Receive Failed");
//     else if (error == OTA_END_ERROR)
//       Serial.println("End Failed");
//   });
// #endif
//   ArduinoOTA.begin();
// #ifdef DEBUG
//   Serial.println("OTA ready");
// #endif
// }

void setupDisplay() {
  Wire.begin(PIN_SDA, PIN_SCK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.dim(true);  // lower brightness
}

void setupRTC() {
  if (!rtc.begin()) {
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

  setupDisplay();

  // setupFilesystem();

#ifdef DEBUG
  Serial.println("setupFilesystem finished");
#endif

  setupRTC();

  display.clearDisplay();
  display.setCursor(0, 14);
  display.setTextSize(2);

  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);
  wifiActive = false;
  display.setTextSize(1);
  display.print("NO WIFI MODE");
  display.display();

  delay(1000);
}

//*************************
// LOOP
//*************************

void loop() {
  fetchData();
  updateScreen();
  sleep();
}
