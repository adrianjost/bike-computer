/* AT Tiny RPM Encode
* Ben Harper April 2014
* LED on digital pin 3 (ATtiny Pin 2)
* IR Encoder input onto digital pin 4 (ATtiny Pin 3)    
*
                                       ________
(PCINT5/RESET/ADC0/dW) PB5         - 1|        |8 - VCC
(PCINT3/XTAL1/CLKI/OC1B/ADC3) PB3  - 2|   AT   |7 - PB2 (SCK/USCK/SCL/ADC1/T0/INT0/PCINT2)
PCINT4/XTAL2/CLKO/OC1B/ADC2) PB4   - 3|  TINY  |6 - PB1 (MISO/DO/AIN1/OC0B/OC1A/PCINT1)
GND                                - 4|  45/85 |5 - 5PB0 (MOSI/DI/SDA/AIN0/OC0A/OC1A/AREF/PCINT0)
                                       --------
In Arduino Terminology:
AT45Pin1 = Arduino's Reset
AT45Pin2 = Arduino's Digital 3 & Analog 3
AT45Pin3 = Arduino's Digital 4 & Analog 2
AT45Pin4 = Arduino's Gnd
AT45Pin5 = Arduino's Digital 0 (PWM) & MOSI
AT45Pin6 = Arduino's Digital 1 (PWM) & MISO
AT45Pin7 = Arduino's Digital 2 & Analog 1 & SCK
AT45Pin8 = Arduino's VCC

Good Info For ATTiny45 With Arduino: http://highlowtech.org/?p=1695
*/

#include "TinyWireS.h"  //For I2C on ATTiny's

#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  pinMode(4, OUTPUT);
}

void loop() {
  digitalWrite(4, LOW);
  TinyWireS_stop_check();
}

void requestEvent() {
  digitalWrite(4, HIGH);
  TinyWireS.send(B11001101);
}