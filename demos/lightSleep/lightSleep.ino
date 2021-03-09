#define PIN_SENSOR 0

#include <ESP8266WiFi.h>        // 1.0.0 - Ivan Grokhotkov
// Low Power Sleep States
extern "C" {
  #include "user_interface.h"
}

void forcedLightSleep() {
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(PIN_SENSOR, GPIO_PIN_INTR_LOLEVEL);
  // only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
  // wifi_fpm_set_wakeup_cb(wakeupCallback); // Set wakeup callback (optional)
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);  // only 0xFFFFFFF, any other value and it won't disconnect the RTC timer
  delay(10);  // it goes to sleep during this delay() and waits for an interrupt
}

void timedLightSleepCallback() {
  // do nothing
  Serial.println("Wake Up");
}
void timedLightSleep(unsigned int sleepMs){
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  extern os_timer_t *timer_list;
  timer_list = nullptr;  // stop (but don't disable) the 4 OS timers
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(PIN_SENSOR, GPIO_PIN_INTR_LOLEVEL);
  wifi_fpm_set_wakeup_cb(timedLightSleepCallback); // set wakeup callback
  // the callback is optional, but without it the modem will wake in 10 seconds then delay(10 seconds)
  // with the callback the sleep time is only 10 seconds total, no extra delay() afterward
  wifi_fpm_open();
  wifi_fpm_do_sleep(sleepMs * 1000);  // Sleep range = 10000 ~ 268,435,454 uS (0xFFFFFFE, 2^28-1)
  delay(sleepMs + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SENSOR, INPUT);
  delay(1000);
}

void loop() {
  Serial.println("Awake");
  timedLightSleep(5000);
}