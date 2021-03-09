#define PIN_SDA 2
#define PIN_SCK 0
// #define PIN_SENSOR 3

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     1

#include <ESP8266WiFi.h>        // 1.0.0 - Ivan Grokhotkov

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Low Power Sleep States
extern "C" {
  #include "user_interface.h"
}

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void wakeupCallback(){
  display.clearDisplay();
  display.setCursor(0, 14);
  display.setTextSize(1);
  display.print("T");
  display.print(millis());
  // display.display();
}

void updateScreen(){
  display.clearDisplay();
  display.setCursor(0, 14);
  display.setTextSize(1);
  display.print("T");
  display.print(millis());
  display.display();
}

void timedLightSleep(unsigned int sleepMs){
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  extern os_timer_t *timer_list;
  timer_list = nullptr;  // stop (but don't disable) the 4 OS timers
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  #ifdef PIN_SENSOR
    gpio_pin_wakeup_enable(PIN_SENSOR, GPIO_PIN_INTR_LOLEVEL);
  #endif
  wifi_fpm_set_wakeup_cb(wakeupCallback); // set wakeup callback
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_open();
  wifi_fpm_do_sleep(sleepMs * 1000);  // Sleep range = 10000 ~ 268,435,454 uS (0xFFFFFFE, 2^28-1)
  delay(sleepMs + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void setup() {
  #ifdef PIN_SENSOR
    pinMode(PIN_SENSOR, INPUT);
  #endif

  Wire.begin(PIN_SDA, PIN_SCK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.dim(true); // lower brightness
  updateScreen();

  delay(1000);
}

void loop() {
  updateScreen();
  timedLightSleep(1000);
}