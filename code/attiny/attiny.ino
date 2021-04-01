#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/atomic.h>

#include "TinyWireS.h"  //For I2C on ATTiny's

#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave

#define PIN_SENSOR 4
#define PIN_WAKEUP_ESP 1
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

#define INTERRUPT_THROTTLE_COOLDOWN 75000  // 50
#define DATA_SIZE 30
#define USED_VALUES_FOR_CALC 10
#define MAX_VALUE_AGE 4000000  // 3000
#define CLEAR_DATA                       \
  for (byte i = 0; i < DATA_SIZE; ++i) { \
    data[i] = 0;                         \
  };

// Variables for the Sleep/power down modes:
volatile boolean f_wdt = true;
volatile boolean sending = false;
volatile byte rotationCountSinceLastSend = 0;
// speed measurment
volatile unsigned long data[DATA_SIZE];
volatile byte lastWriteIndex = 0;

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  setup_watchdog(6);

  pinMode(PIN_WAKEUP_ESP, OUTPUT);
  digitalWrite(PIN_WAKEUP_ESP, LOW);

  pinMode(PIN_SENSOR, INPUT);

  sbi(GIMSK, PCIE);                              // Enable Pin Change Interrupts
  sbi(PCMSK, digitalPinToPCMSKbit(PIN_SCL));     // Use PCINTX as interrupt pin
  sbi(PCMSK, digitalPinToPCMSKbit(PIN_SENSOR));  // Use PCINTX as interrupt pin
}

void loop() {
  TinyWireS_stop_check();
  if (f_wdt == true) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    f_wdt = false;      // reset flag
    digitalWrite(PIN_WAKEUP_ESP, HIGH);
    delay(50);
    digitalWrite(PIN_WAKEUP_ESP, LOW);
  }
  if (!sending) {
    sleep();  // Send the unit to sleep
  }
}

byte speed = 0;
void updateSpeed() {
  byte endIndex = lastWriteIndex;
  unsigned long end = data[endIndex];
  byte startIndex;
  unsigned long start;
  byte usedValues = 0;
  for (byte i = 1; i <= USED_VALUES_FOR_CALC; i++) {
    byte valIndex = (endIndex + DATA_SIZE - i) % DATA_SIZE;
    unsigned long val = data[valIndex];
    if ((end - val) > (MAX_VALUE_AGE)) {
      // value too old
      break;
    }
    usedValues++;
    startIndex = valIndex;
    start = val;
  }
  if (start == end || usedValues == 0) {
    speed = 0;  // TODO: does not trigger somehow if I don't send any new signal, the speed stays unchanged
  } else {
    speed = (byte)((usedValues * 20) / ((float)(end - start) / 1000000));
  }
}

void requestEvent() {
  sending = true;

  updateSpeed();

  // TODO [$6065a24faecd480008c540fc]: read real battery voltage
  //
  // - [ ] use analog input to measure voltage
  // - [ ] map voltage to calibration data because min & max depend on battery
  // - [ ] calculating the current battery level
  unsigned long now = micros() / 1000;
  byte battery = (now / 1000) % 4;

  byte rotationCount = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    rotationCount = rotationCountSinceLastSend;
    rotationCountSinceLastSend = 0;
  }

  TinyWireS.send(speed);
  TinyWireS.send((rotationCount << 4) | battery);

  sending = false;
}

// set system into the sleep state
void sleep() {
  // prepeare for sleep
  pinMode(PIN_WAKEUP_ESP, INPUT);  // Set the ports to be inputs - saves more power

  // timers (micros, millis) are not increasing in other sleep modes because the clock get's stopped
  set_sleep_mode(SLEEP_MODE_IDLE);  // replaces above statement
  power_adc_disable();
  power_timer1_disable();  // timer0 is required to be alive for micros to work

  cli();  // Disable interrupts
  if (!sending) {
    sleep_enable();  // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
    sei();           // Enable interrupts
    sleep_cpu();     // sleep
  }

  // Wake-Up
  cli();                            // Disable interrupts
  sleep_disable();                  // Clear SE bit
  power_adc_enable();               // switch Analog to Digitalconverter ON
  power_timer1_enable();            // switch everything on
  pinMode(PIN_WAKEUP_ESP, OUTPUT);  // Set the ports to be output again
  sei();                            // Enable interrupts
}

// 0=16ms, 1=32ms,2=64ms,3=128ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9=8sec
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

// onyl one ISR for all Pin Interrupts exists
// => detect manually what has changed
ISR(PCINT0_vect) {
  // interrupt on PIN_SENSOR
  // we don't need to do any change analysis, because duplicate calls get filtered by the throttling. Also the interrupt is only triggered when the uC sleeps, and thus a new wakeup call is triggered. For that reason, we do potentially miss the reset of the sensor and would be unable to detect a change when the sensor is triggered next time.
  if (INTERRUPT_PIN_PORT & (1 << PIN_SENSOR)) {
    unsigned long interruptTime = micros();
    if (interruptTime - data[lastWriteIndex] < INTERRUPT_THROTTLE_COOLDOWN) {
      return;
    }
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      lastWriteIndex = (lastWriteIndex + 1) % DATA_SIZE;
      data[lastWriteIndex] = interruptTime;
      rotationCountSinceLastSend += 1;
    }
  }
}
