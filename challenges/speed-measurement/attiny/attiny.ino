// #define DEBUG
#define SENSOR_PIN 3
#define SENSOR_INTERRUPT RISING
#define INTERRUPT_THROTTLE_COOLDOWN 50

#define WHEEL_CIRCUMFERENCE 2100

#define DATA_SIZE 20
#define USED_VALUES_FOR_CALC 10
#define MAX_VALUE_AGE 3000
#define CLEAR_DATA                       \
  for (byte i = 0; i < DATA_SIZE; ++i) { \
    data[i] = 0;                         \
  };
unsigned long data[DATA_SIZE];
byte lastWriteIndex = 0;

void handleInterrupt() {
  unsigned long interruptTime = millis();
  if (interruptTime - data[lastWriteIndex] < INTERRUPT_THROTTLE_COOLDOWN) {
    return;
  }
  lastWriteIndex = (lastWriteIndex + 1) % DATA_SIZE;
  data[lastWriteIndex] = interruptTime;
}

float speed = 0;
void updateSpeed() {
  byte endIndex = lastWriteIndex;
  unsigned long end = data[endIndex];
  byte startIndex;
  unsigned long start;
  byte usedValues = 0;
  for (byte i = 1; i <= USED_VALUES_FOR_CALC; i++) {
    byte valIndex = (endIndex + DATA_SIZE - i) % DATA_SIZE;
    unsigned long val = data[valIndex];
    if ((end - val) > (MAX_VALUE_AGE)) {
      // value too old
      break;
    }
    usedValues++;
    startIndex = valIndex;
    start = val;
  }
#ifdef DEBUG
  Serial.print("     i ");
  Serial.print(startIndex);
  Serial.print(" - ");
  Serial.println(endIndex);
  Serial.print("     v ");
  Serial.print(start);
  Serial.print(" - ");
  Serial.println(end);
#endif

  if (start == end || usedValues == 0) {
    speed = 0;
  } else {
    speed = (float)((usedValues * WHEEL_CIRCUMFERENCE) / (end - start)) * 3.6;
  }
}

void setup() {
  CLEAR_DATA
  Serial.begin(9600);
  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), handleInterrupt, SENSOR_INTERRUPT);
#ifdef DEBUG
  Serial.print("GO ----------------------- ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
#endif
}

void loop() {
  updateSpeed();
  Serial.println(speed);
  delay(1000);
}
