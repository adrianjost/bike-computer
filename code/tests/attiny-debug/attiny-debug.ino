#define PIN_SDA 2
#define PIN_SCK 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET 1

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>

#define WHEEL_CIRCUMFERENCE 2000

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

  // disable wifi
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(0xFFFFFFF);
}

String message;

void updateScreen() {
  Wire.requestFrom(0x4, 2);  // request x bytes from slave device #0x...
  byte rPer20s = 0;
  byte batteryAndRotationCount = 0;
  byte read = 0;
  while (Wire.available()) {
    if (read == 0) {
      rPer20s = Wire.read();
    } else if (read == 1) {
      batteryAndRotationCount = Wire.read();
    } else {
      Wire.read();
    }
    read++;
  }

  byte battery = batteryAndRotationCount & B00001111;
  byte rotationCount = ((batteryAndRotationCount & B11110000) >> 4);

  float speed = (float)(rPer20s * WHEEL_CIRCUMFERENCE * 3.6) / 20000.0;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(speed);
  display.println(battery);
  display.println(rotationCount);
  display.display();
}

bool state = 0;
void loop() {
  updateScreen();
  delay(1000);
}
