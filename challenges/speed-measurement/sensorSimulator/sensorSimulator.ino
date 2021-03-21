/**
 * A simple PWM/signal generator that permits setting of the frequency
 * in Herz and the pulse width (duty cycle). It uses a bit banging method
 * to generate the signal. This sketch can also be used to generate a PWM 
 * signal on any digital pin.
 * 
 * Author: Mario Gianota July 2020
 */
#define OUTPUT_PIN 2    // PWM/Signal output pin
#define DUTY_CYCLE 5.0  // Duty cycle (pulse width) percentage

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);
}

void writeFrequency(float frequency) {  // Frequency in Herz
  // Calculate the period and the amount of time the output is on for (HIGH) and
  // off for (LOW).
  double period = 1000000 / frequency;
  double offFor = period - (period * (DUTY_CYCLE / 100));
  double onFor = period - offFor;

  if (period > 16383) {
    // If the period is greater than 16383 then use the millisecond timer delay,
    // otherwise use the microsecond timer delay. Currently, the largest value that
    // will produce an accurate delay for the microsecond timer is 16383.

    digitalWrite(OUTPUT_PIN, HIGH);
    delay((long)onFor / 1000);

    digitalWrite(OUTPUT_PIN, LOW);
    delay((long)offFor / 1000);
  } else {
    digitalWrite(OUTPUT_PIN, HIGH);
    delayMicroseconds((long)onFor);

    digitalWrite(OUTPUT_PIN, LOW);
    delayMicroseconds((long)offFor);
  }
}

void loop() {
  writeFrequency(((float)(((millis() / 10000) % 20) + 1) / 4));
}
