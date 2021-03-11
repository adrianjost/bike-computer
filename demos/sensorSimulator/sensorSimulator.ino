#define SENSOR_PIN 2
#define SENSOR_HIGH 30
#define WHEEL_CIRCUMFERENCE 2100

#define SENSOR_DURATION_05 1512
#define SENSOR_DURATION_10 756
#define SENSOR_DURATION_20 378
#define SENSOR_DURATION_30 252
#define SENSOR_DURATION_40 189
#define SENSOR_DURATION_50 151
#define SENSOR_DURATION_60 126
#define SENSOR_DURATION_70 108
#define SENSOR_DURATION_80 95
#define SENSOR_DURATION_90 84

void setup() {
  pinMode(SENSOR_PIN, OUTPUT);
}

void simulateSpeed(int roundDuration) {
  digitalWrite(SENSOR_PIN, HIGH);
  delay(SENSOR_HIGH);
  digitalWrite(SENSOR_PIN, LOW);
  delay(roundDuration - SENSOR_HIGH);
}

void loop() {
  switch((millis() / 10000) % 10){
    case 0: simulateSpeed(SENSOR_DURATION_05); break;
    case 1: simulateSpeed(SENSOR_DURATION_10); break;
    case 2: simulateSpeed(SENSOR_DURATION_20); break;
    case 3: simulateSpeed(SENSOR_DURATION_30); break;
    case 4: simulateSpeed(SENSOR_DURATION_40); break;
    case 5: simulateSpeed(SENSOR_DURATION_50); break;
    case 6: simulateSpeed(SENSOR_DURATION_60); break;
    case 7: simulateSpeed(SENSOR_DURATION_70); break;
    case 8: simulateSpeed(SENSOR_DURATION_80); break;
    case 9: simulateSpeed(SENSOR_DURATION_90); break;
  }
}
