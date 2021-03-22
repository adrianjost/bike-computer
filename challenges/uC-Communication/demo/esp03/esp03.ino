#define PIN_SDA 2
#define PIN_SCK 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 1

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

String message;

void updateScreen() {
  Wire.requestFrom(0x4, 2);  // request 2 bytes from slave device #0x13
  byte speed = 0;
  byte second = 0;
  byte read = 0;
  while (Wire.available()) {
    if (read == 0) {
      speed = Wire.read();
    } else if (read == 1) {
      second = Wire.read();
    } else {
      Wire.read();
    }
    read++;
  }

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

bool state = 0;
void loop() {
  updateScreen();
  delay(2000);
}