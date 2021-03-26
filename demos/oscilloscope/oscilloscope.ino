#define PIN_TEST A5

void setup() {
  Serial.begin(57600);
  pinMode(PIN_TEST, INPUT);
}

void loop() {
  Serial.println(digitalRead(PIN_TEST));
}