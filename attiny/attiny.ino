#define SENSOR_PIN 2
#define SENSOR_INTERRUPT RISING
#define SENSOR_TRIGGERED HIGH
#define INTERRUPT_THROTTLE_COOLDOWN 125
#define VALIDATION_LOOPS 20
#define INTERRUPT_DEBOUNCE_THRESHOLD 0.75

#define WHEEL_CIRCUMFERENCE 2100

struct SensorData {
  unsigned long start;
  unsigned long end;
  byte count;
};

/*
dataToggle is used as a readonly switch to prevent that
data that is used for the speed calculation, is manipulated
by the interrupt during the calculation.

true = use A for interrupt writes
false = use B for interrupt writes
reading data should be vice versa, to read from the unmutateable dataset
*/
bool dataToggle = true;
volatile SensorData dataA{0, 0, 0};
volatile SensorData dataB{0, 0, 0};

bool validateSensorState(bool expected) {
  byte valid = 0;
  for (byte i = 0; i < VALIDATION_LOOPS; i++) {
    if (digitalRead(SENSOR_PIN) == expected) {
      valid += 1;
    }
  }
  return valid > (VALIDATION_LOOPS * INTERRUPT_DEBOUNCE_THRESHOLD);
}

void handleInterrupt() {
  unsigned long interruptTime = millis();
  unsigned long lastInterrupt = dataToggle ? dataA.end : dataB.end;
  if (interruptTime - lastInterrupt < INTERRUPT_THROTTLE_COOLDOWN) {
    return;
  }
  if (!validateSensorState(SENSOR_TRIGGERED)) {
    return;
  }

  if (dataToggle) {
    dataA.end = interruptTime;
    dataA.count += 1;
  } else {
    dataB.end = interruptTime;
    dataB.count += 1;
  }
}

float speed = 0;
void updateSpeed() {
  dataToggle = !dataToggle;

  unsigned long start = 0;
  unsigned long end = 0;
  byte count = 0;

  if (!dataToggle) {
    start = dataA.start;
    end = dataA.end;
    count = dataA.count;
    dataB.start = end;
    dataA = { 0, 0, 0};
  } else {
    start = dataB.start;
    end = dataB.end;
    count = dataB.count;
    dataA.start = end;
    dataB = { 0, 0, 0};
  }

  if (start == end) {
    speed = 0;
  } else {
    speed = (float)((count * WHEEL_CIRCUMFERENCE) / (end - start)) * 3.6;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handleInterrupt, SENSOR_INTERRUPT);
  // Serial.print("GO ----------------------- ");
  // Serial.print(__DATE__);
  // Serial.print(" ");
  // Serial.println(__TIME__);
}

void loop() {
  updateSpeed();
  Serial.println(speed);
  delay(1000);
}
