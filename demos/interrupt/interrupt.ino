void ICACHE_RAM_ATTR handleInterrupt();

const byte interruptPin = 13;
volatile byte interruptCounter = 0;
int numberOfInterrupts = 0;
 

void setup() {
 
  Serial.begin(115200);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);
  delay(1000);
}
 
void handleInterrupt() {
  interruptCounter++;
}
 
void loop() {
 
  if(interruptCounter>0){
 
      interruptCounter--;
      numberOfInterrupts++;
 
      Serial.print("An interrupt has occurred. Total: ");
      Serial.println(numberOfInterrupts);
      delay(500);
  }
 
}