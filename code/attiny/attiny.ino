// PIN DEFINITIONS
#define PIN_WAKEUP_ESP 1
#define PIN_HALL_SENSOR 4
#define WAKEUP_PIN_HALL_SENSOR PCINT4
#define PIN_BATTERY_VOLTAGE A1

// IMPORTS
// Low Power Modes
#include <avr/interrupt.h>
#include <avr/sleep.h>
// I2C Communication
#include "TinyWireS.h"         //For I2C on ATTiny's
#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave

// HELPER
// Routines to set and clear bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// GLOBAL STATE
byte battery = 2;
float speed = 27.3;

// Wake-Up-Reasons
volatile boolean wakupWatchdog = 0;

/* USE-CASES */

// SENSOR
void updateBatteryState() {
  // TODO read battery state from analog pin and adjust according to saved calibration
}

void updateSpeed() {
}

void requestEvent() {
  updateBatteryState();
  updateSpeed();
  byte speedBase = (byte)speed;
  byte speedDecimal = speed * 10 - (speedBase * 10);
  byte first = speedBase;
  byte second = (speedDecimal << 4) | battery;
  TinyWireS.send(first);
  TinyWireS.send(second);
}

// WAKE-UP ESP
void wakeUpESP() {
  digitalWrite(PIN_WAKEUP_ESP, HIGH);
  delay(3);
  digitalWrite(PIN_WAKEUP_ESP, LOW);
}

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  wakupWatchdog = 1;  // set global flag
}

// POWER SAVING

void disableOutputs() {
  pinMode(PIN_WAKEUP_ESP, INPUT)
}

void enableOutputs() {
  pinMode(PIN_WAKEUP_ESP, OUTPUT);
  pinMode(PIN_HALL_SENSOR, INPUT);
  pinMode(PIN_BATTERY_VOLTAGE, INPUT);
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  sbi(GIMSK, PCIE);                    // Enable Pin Change Interrupts
  sbi(PCMSK, WAKEUP_PIN_HALL_SENSOR);  // Use PB4 as interrupt pin

  disableOutputs();                     // Set the ports to be inputs - saves more power
  cbi(ADCSRA, ADEN);                    // switch Analog to Digitalconverter OFF
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // sleep mode is set here
  sleep_mode();                         // sleep
  sbi(ADCSRA, ADEN);                    // switch Analog to Digitalconverter ON
  enableOutputs();                      // Set the ports to be output again
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec
void setup_watchdog(int ii) {
  byte bb;
  int ww;
  if (ii > 9) ii = 9;
  bb = ii & 7;
  if (ii > 7) bb |= (1 << 5);
  bb |= (1 << WDCE);
  ww = bb;

  MCUSR &= ~(1 << WDRF);
  // start timed sequence
  WDTCR |= (1 << WDCE) | (1 << WDE);
  // set new watchdog timeout value
  WDTCR = bb;
  WDTCR |= _BV(WDIE);
}

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  sei();  // Enable interrupts
  setup_watchdog(6);
}

void loop() {
  TinyWireS_stop_check();

  if (wakupWatchdog == 1) {
    wakupWatchdog = 0;
    wakeUpESP();
  }
  system_sleep();
}
