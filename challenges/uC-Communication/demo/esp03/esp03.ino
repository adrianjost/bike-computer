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

String message;

void updateScreen() {
  Wire.requestFrom(0x13, 2);  // request 2 bytes from slave device #0x13

  int i = 0;
  unsigned int readout = 0;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(millis());
  while (Wire.available()) {
    byte c = Wire.read();
    display.println(c);
    if (i == 0) {
      readout = c;
    } else {
      readout = readout << 8;
      readout = readout + c;
    }
    i++;
  }
  display.println(readout);
  display.display();
}

bool state = 0;
void loop() {
  updateScreen();
  delay(2000);
}