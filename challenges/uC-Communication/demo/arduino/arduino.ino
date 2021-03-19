#include <Wire.h>

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output^
  Serial.println("GO...");
}

void loop() {
  Wire.requestFrom(4, 2);

  while (Wire.available()) {     // slave may send less than requested
    byte c = (byte)Wire.read();  // receive a byte as character
    Serial.println(c, BIN);      // print the character
  }

  delay(2000);
}
