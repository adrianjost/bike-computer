#define PIN_SENSOR 3

#include <ESP8266WiFi.h>        // 1.0.0 - Ivan Grokhotkov
// Low Power Sleep States
extern "C" {
  #include "user_interface.h"
}

void forcedLightSleep() {
  WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  gpio_pin_wakeup_enable(PIN_SENSOR, GPIO_PIN_INTR_HILEVEL);
  // only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
  // wifi_fpm_set_wakeup_cb(wakeupCallback); // Set wakeup callback (optional)
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);  // only 0xFFFFFFF, any other value and it won't disconnect the RTC timer
  delay(2000);  // it goes to sleep during this delay() and waits for an interrupt
}


// void forcedLightSleep() {
//   // ATTENTION: disables internal clock, so millis() isn't counting up anymore
//   WiFi.mode(WIFI_OFF);  // you must turn the modem off; using disconnect won't work
//   wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
//   #ifdef PIN_SENSOR
//     gpio_pin_wakeup_enable(PIN_SENSOR, WAKEUP_SENSOR);
//   #endif
//   #ifdef PIN_BUTTON
//     gpio_pin_wakeup_enable(PIN_BUTTON, WAKEUP_BUTTON);
//   #endif
//   // only LOLEVEL or HILEVEL interrupts work, no edge, that's an SDK or CPU limitation
//   // wifi_fpm_set_wakeup_cb(handleInterrupt); // Set wakeup callback (optional)
//   wifi_fpm_open();
//   wifi_fpm_do_sleep(0xFFFFFFF);  // only 0xFFFFFFF, any other value and it won't disconnect the RTC timer
//   delay(10); // it goes to sleep during this delay() and waits for an interrupt
// }

void setup(){
  
}

void loop(){
  forcedLightSleep();
}