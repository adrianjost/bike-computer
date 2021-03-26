#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include "TinyWireS.h"  //For I2C on ATTiny's

#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave

#define PIN_LED 1
#define PIN_SENSOR 4
#define PIN_SCL 2
#define PIN_SDA 0

#define INTERRUPT_PIN_PORT PINB

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
volatile byte rotationCountSinceLastSend = 0;

// float speed = 36.4;

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  // setup_watchdog(8);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_SENSOR, INPUT);

  digitalWrite(PIN_LED, HIGH);
  delay(100);
  digitalWrite(PIN_LED, LOW);
}

void loop() {
  TinyWireS_stop_check();
  // if (f_wdt == true) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
  //   f_wdt = false;      // reset flag

  //   digitalWrite(PIN_LED, HIGH);
  //   delay(750);
  //   digitalWrite(PIN_LED, LOW);
  // }
  // if (!sending) {
  system_sleep();  // Send the unit to sleep
  digitalWrite(PIN_LED, HIGH);
  delay(50);
  digitalWrite(PIN_LED, LOW);
  delay(50);
  // }
}

void requestEvent() {
  sending = true;

  // byte speedBase = (byte)speed;
  // byte speedDecimal = (speed * 10) - (speedBase * 10);

  // TODO use real battery calculation
  unsigned long now = millis();
  byte speedBase = (now / 1000) % 100;
  byte speedDecimal = (now / 500) % 10;
  byte battery = (now / 1000) % 4;

  byte rotationCount = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rotationCount = rotationCountSinceLastSend;
    rotationCountSinceLastSend = 0;
  }

  // TODO send interpolated rotations per fixed interval instead of speed
  // so we don't need to know the wheel circumference here
  TinyWireS.send(speedBase);
  TinyWireS.send((speedDecimal << 4) | battery);
  TinyWireS.send(rotationCount);

  sending = false;
}

// set system into the sleep state
void system_sleep() {
  sbi(GIMSK, PCIE);    // Enable Pin Change Interrupts
  sbi(PCMSK, PCINT2);  // Use PCINTX as interrupt pin
  sbi(PCMSK, PCINT4);  // Use PCINTX as interrupt pin
  cbi(ADCSRA, ADEN);   // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // replaces above statement

  sleep_enable();  // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();           // Enable interrupts
  sleep_cpu();     // sleep

  cli();               // Disable interrupts
  cbi(PCMSK, PCINT2);  // Turn off PCINTX as interrupt pin
  cbi(PCMSK, PCINT4);  // Turn off PCINTX as interrupt pin
  sleep_disable();     // Clear SE bit
  sbi(ADCSRA, ADEN);   // switch Analog to Digitalconverter ON

  sei();  // Enable interrupts                             // Enable interrupts

  // // pinMode(PIN_LED, INPUT);  // Set the ports to be inputs - saves more power

  // // sbi(GIMSK, PCIE);                              // Enable Pin Change Interrupts
  // // sbi(PCMSK, digitalPinToPCMSKbit(PIN_SCL));     // Used to wakeup
  // // sbi(PCMSK, digitalPinToPCMSKbit(PIN_SENSOR));  // Used to wakeup

  // sbi(GIMSK, PCIE);
  // sbi(PCMSK, PCINT4);

  // // cbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter OFF

  // // go to sleep, except if there is a new I2C request
  // set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // cli();
  // if (!sending) {
  //   sleep_enable();
  //   sei();
  //   sleep_cpu();
  //   sleep_disable();
  // }
  // sei();

  // // sbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter ON

  // // pinMode(PIN_LED, OUTPUT);  // Set the ports to be output again
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
  // ISR(WDT_vect) {
  f_wdt = true;
}

// onyl one ISR for all Pin Interrupts exists
// => detect manually what has changed
volatile byte oldPort = 0x00;
byte interruptMask = (1 << PIN_SCL) | (1 << PIN_SENSOR);
ISR(PCINT0_vect) {
  rotationCountSinceLastSend += 1;

  // // get the new pin states for port
  // byte newPort = INTERRUPT_PIN_PORT;
  // // byte newPort = *portInputRegister(digitalPinToPort(PIN_SENSOR));

  // // compare with the old value to detect a rising or falling
  // byte change = newPort ^ oldPort;

  // // detect rising pins
  // byte rising = change & newPort;

  // // detect falling pins
  // // byte falling = change & oldPort; // not used

  // // check which pins are triggered, compared with the settings
  // byte changed = 0x00;
  // changed |= (rising & interruptMask);
  // // changed |= (falling & interruptMask);

  // // save the new state for next comparison
  // oldPort = newPort;

  // // interrupt on PIN_SCL
  // // if (changed & (1 << PIN_SCL)) {
  // //   sending = true;
  // // }

  // // interrupt on PIN_SENSOR
  // if (changed & (1 << PIN_SENSOR)) {
  //   rotationCountSinceLastSend += 1;
  // }
}
