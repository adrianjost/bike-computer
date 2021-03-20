#include <avr/interrupt.h>
#include <avr/sleep.h>

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// *********** Define I/O Pins **********************
const int switchPin = 4;
const int statusLED = 1;

// *********** Code *********************************

// Variables for the Sleep/power down modes:
volatile boolean interrupt_triggered = 1;

void setup() {
  pinMode(switchPin, INPUT);
  digitalWrite(switchPin, HIGH);
  pinMode(statusLED, OUTPUT);
}  // setup

void sleep() {
  sbi(GIMSK, PCIE);    // Enable Pin Change Interrupts
  sbi(PCMSK, PCINT4);  // Use PB4 as interrupt pin
  cbi(ADCSRA, ADEN);   // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // replaces above statement

  sleep_enable();  // Sets the Sleep Enable bit in the MCUCR Register (SE BIT)
  sei();           // Enable interrupts
  sleep_cpu();     // sleep

  cli();               // Disable interrupts
  cbi(PCMSK, PCINT4);  // Turn off PB4 as interrupt pin
  sleep_disable();     // Clear SE bit
  sbi(ADCSRA, ADEN);   // switch Analog to Digitalconverter ON

  sei();  // Enable interrupts
}  // sleep

ISR(PCINT0_vect) {
  interrupt_triggered = 1;  // set global flag
}

void loop() {
  if (interrupt_triggered == 1) {  // wait for interrupt
    interrupt_triggered = 0;       // reset flag
    digitalWrite(statusLED, HIGH);
    delay(100);
    digitalWrite(statusLED, LOW);
  }
  sleep();
}