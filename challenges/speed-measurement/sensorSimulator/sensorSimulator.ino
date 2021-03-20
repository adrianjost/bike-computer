#define SENSOR_PIN 3
#define DUTY_CYCLE 5
#define SENSOR_HIGH 10
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
#define SENSOR_DURATION_99 75

#define DEBUG

void setup() {
  pinMode(SENSOR_PIN, OUTPUT);
#ifdef DEBUG
  Serial.begin(9600);
#endif
}

void simulateSpeed(int periodMs) {
  int dutyCycleMs = DUTY_CYCLE * periodMs;
  digitalWrite(SENSOR_PIN, HIGH);
#ifdef DEBUG
  Serial.print(1);
#endif
  delayMicroseconds(dutyCycleMs * 1000);
  digitalWrite(SENSOR_PIN, LOW);
#ifdef DEBUG
  Serial.print(0);
#endif
  delayMicroseconds((periodMs - dutyCycleMs) * 1000);
}

void loop() {
  switch ((millis() / 10000) % 8) {
    case 0:
      simulateSpeed(SENSOR_DURATION_05);
      break;
    case 1:
      simulateSpeed(SENSOR_DURATION_10);
      break;
    case 2:
      simulateSpeed(SENSOR_DURATION_20);
      break;
    case 3:
      simulateSpeed(SENSOR_DURATION_30);
      break;
    case 4:
      simulateSpeed(SENSOR_DURATION_40);
      break;
    case 5:
      simulateSpeed(SENSOR_DURATION_50);
      break;
    case 6:
      simulateSpeed(SENSOR_DURATION_60);
      break;
    case 7:
      simulateSpeed(SENSOR_DURATION_70);
      break;
    case 8:
      simulateSpeed(SENSOR_DURATION_80);
      break;
    case 9:
      simulateSpeed(SENSOR_DURATION_90);
      break;
  }
}
