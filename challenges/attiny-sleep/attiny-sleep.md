# Attiny Sleep

## Goal

Reduce power consumption of Attiny as much as possible while maintaining

- to wake up every n seconds
- handle speed sensor signal
- answer to data request from the ESP

## Attiny Power Consumption

| Mode                | CPU Frequenzy | Power Consumption |
| ------------------- | ------------- | ----------------- |
| Regular             | 1 MHz         | 1.7mA             |
| Regular             | 8 MHz         | 6.6 mA            |
| Regular             | 16 MHz        | 12 mA             |
| SLEEP_MODE_PWR_DOWN | 1 MHz         | 6 uA              |
| SLEEP_MODE_PWR_DOWN | 8 MHz         | 6 uA              |
| SLEEP_MODE_PWR_DOWN | 16 MHz        | 6 uA              |

## Code Pieces

### System Sleep

```arduino
// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// set system into the sleep state
// system wakes up when wtchdog is timed out
void system_sleep() {
  cbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter OFF

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);  // sleep mode is set here
  sleep_enable();

  sleep_mode();  // System actually sleeps here

  sleep_disable();  // System continues execution here when watchdog timed out

  sbi(ADCSRA, ADEN);  // switch Analog to Digitalconverter ON
}
```

### Watch-Dog Wake-Up

```arduino
// Variables for the Sleep/power down modes:
volatile boolean f_wdt = 1;

void setup() {
  setup_watchdog(8);  // only need to be called once
}

// the loop routine runs over and over again forever:
void loop() {
  if (f_wdt == 1) {  // wait for timed out watchdog / flag is set when a watchdog timeout occurs
    f_wdt = 0;       // reset flag

    // do yout thing and go to sleep
  }
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
  f_wdt = 1;  // set global flag
}
```

### Interrupt Wake-Up

```arduino
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Routines to set and claer bits (used in the sleep code)
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

// Variables for the Sleep/power down modes:
volatile boolean interrupt_triggered = 1;

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
    // do stuff
  }
  sleep();
}
```

## Usefull Links / Tutorials / Resources

https://www.re-innovation.co.uk/docs/sleep-modes-on-attiny85/
https://bigdanzblog.wordpress.com/2014/08/10/attiny85-wake-from-sleep-on-pin-state-change-code-example/
