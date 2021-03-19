/*
  ESP-03 PIN Test Blink demo
*/

#define PIN 3

void setup() {
  pinMode(PIN, OUTPUT);
}

void loop() {
  digitalWrite(PIN, HIGH);
  delay(100);
  digitalWrite(PIN, LOW);
  delay(200);
}
