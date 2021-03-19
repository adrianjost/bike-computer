# uC-Communication - i2C between ESP8266 & Attiny45

## Gathered Infos

- master need to support clock-stretching. Arduino Wire Lib does
- Attiny does not support arduino WireLib and has no native i2C support => [new lib required](https://github.com/rambo/TinyWire/tree/master)

## Serial Communication

### Known Limitations

- interrupts do not work while a communication is running
- huge size, so the Attiny45 is already near his memory/flash limit
- needs two additional pins on the ESP8266

### Code

#### Attiny45

```arduino
#include <SoftwareSerial.h>

#define RX 3
#define TX 4

#define MESSAGE_START_FLAG B11111111

SoftwareSerial Serial(RX, TX);

void setup() {
  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis();
  byte speed = (now / 1000) % 100;
  byte speedDecimal = (now / 500) % 10;
  byte battery = (now / 1000) % 4;

  byte first = speed;
  byte second = (speedDecimal << 4) | battery;
  Serial.write(MESSAGE_START_FLAG);
  Serial.write(first);
  Serial.write(second);
  delay(1891);
}
```

#### ESP8266-03

```arduino
#define PIN_SDA 2
#define PIN_SCK 14

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 1

#define MESSAGE_START_FLAG B11111111

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <Wire.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Wire.begin(PIN_SDA, PIN_SCK);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.dim(true);  // lower brightness

  Serial.begin(9600);

  display.invertDisplay(1);
  delay(500);
  display.invertDisplay(0);
  delay(500);
}

void updateScreen() {
  byte available = Serial.available();
  if (available >= 3) {
    byte first;
    byte i = 0;
    do {
      first = Serial.read();
      i++;
    } while (first != MESSAGE_START_FLAG && available - i >= 3 && available - i < 5);  // TODO check condition
    // goal is, to get the last section of 3 bytes that starts with the MESSAGE_START_FLAG

    if (first != MESSAGE_START_FLAG) {
      // no valid message available
      return;
    }
    byte speed = Serial.read();
    byte second = Serial.read();
    byte battery = second & B00001111;
    byte speedFraction = ((second & B11110000) >> 4);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(speed);
    display.print(".");
    display.println(speedFraction);
    display.println(battery);
    display.display();
  }
}

bool state = 0;
void loop() {
  updateScreen();
  delay(2000);
}
```

## i2C

### Known Limitations

- interrupts do not work while a communication is running
- huge size, so the Attiny45 is already near his memory/flash limit
- needs two additional pins on the ESP8266

### Code

#### Attiny45

```arduino
#include "TinyWireS.h"  //For I2C on ATTiny's

#define I2C_SLAVE_ADDRESS 0x4  // Address of this device as an I2C slave

void setup() {
  TinyWireS.begin(I2C_SLAVE_ADDRESS);  // join i2c network
  TinyWireS.onRequest(requestEvent);   // Sets Functional to call on I2C request

  pinMode(4, OUTPUT);
}

void loop() {
  digitalWrite(4, LOW);
  TinyWireS_stop_check();
}

void requestEvent() {
  digitalWrite(4, HIGH);
  TinyWireS.send(B11001101);
}
```

#### Arduino Uno

```arduino
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

```

## Usefull Links / Tutorials / Resources

https://thewanderingengineer.com/2014/02/17/attiny-i2c-slave/
https://github.com/DzikuVx/attiny_photoresistor_i2c
