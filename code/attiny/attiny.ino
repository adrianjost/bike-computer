#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "TinyWireS.h"  //For I2C on ATTiny's

#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave
#define PIN_LED 1
#define INTERRUPT_PIN_SCL PCINT2

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Variables for the Sleep/power down modes:
volatile boolean f_wdt = true;
volatile boolean sending = false;

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  setup_watchdog(8);

  pinMode(PIN_LED, OUTPUT);

  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  TinyWireS_stop_check();
  if (f_wdt == true) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    f_wdt = false;      // reset flag

    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
  }
  if (!sending) {
    system_sleep();  // Send the unit to sleep
  }
}

void requestEvent() {
  sending = true;
  digitalWrite(PIN_LED, HIGH);
  unsigned long now = millis();
  byte speed = (now / 1000) % 100;
  byte speedDecimal = (now / 500) % 10;
  byte battery = (now / 1000) % 4;

  byte first = speed;
  byte second = (speedDecimal << 4) | battery;
  TinyWireS.send(first);
  TinyWireS.send(second);

  delay(50);
  digitalWrite(PIN_LED, LOW);
  sending = false;
}

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  pinMode(PIN_LED, INPUT);  // Set the ports to be inputs - saves more power

  sbi(GIMSK, PCIE);               // Enable Pin Change Interrupts
  sbi(PCMSK, INTERRUPT_PIN_SCL);  // Use I2C SCL Pin to wakeup

  cbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter OFF

  // go to sleep, except if there is a new I2C request
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();
  if (!sending) {
    sleep_enable();
    sei();
    sleep_cpu();
    sleep_disable();
  }
  sei();

  sbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter ON

  pinMode(PIN_LED, OUTPUT);  // Set the ports to be output again
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

// Watchdog Interrupt Service / is executed when watchdog timed out
ISR(WDT_vect) {
  f_wdt = true;
}

ISR(PCINT0_vect) {
  sending = true;
}