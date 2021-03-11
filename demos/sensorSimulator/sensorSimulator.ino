#define SENSOR_PIN 2
#define SENSOR_HIGH 30
#define WHEEL_CIRCUMFERENCE 2100

#define SENSOR_DURATION 378 // ~20km/h
// #define SENSOR_DURATION 302 // ~25km/h

void setup() {
  pinMode(SENSOR_PIN, OUTPUT);
}

void loop() {
  digitalWrite(SENSOR_PIN, HIGH);
  delay(SENSOR_HIGH);
  digitalWrite(SENSOR_PIN, LOW);
  delay(SENSOR_DURATION - SENSOR_HIGH);
}
