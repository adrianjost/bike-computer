# Bike-Computer

## Development Notes

Currently using ESP-01S but ESP-03 might be a bit better suited for the final PCB cause it is smaller and has more PINs available. However I couldn't find a seller yet.

### Pin Compatibility

| Arduino Pin | IN  | OUT |  SDA | SCK | note                                                                                   |
| :---------: | :-: | :-: | :--: | :-: | -------------------------------------------------------------------------------------- |
|      0      | ✅  | ✅  |  ❌  | ✅  | is pulled high during normal operation, PULL-UP is best practice                       |
|      1      | ✅  | ✅  |  ❌  | ❌  | input seems to have internal PULL-UP (needs testing, if true, add external to be safe) |
|      2      | ✅  | ✅  |  ✅  | ❌  | can’t be low at boot                                                                   |
|      3      | ✅  | ✅  |  ❌  | ❌  |                                                                                        |

![ESP8266-01 Pinout & Boot Modes](https://i.pinimg.com/originals/cf/7e/8d/cf7e8de45255400203f46996d8af9603.png)

### Usefull Links / Tutorials

https://lastminuteengineers.com/oled-display-arduino-tutorial/
https://github.com/adafruit/Adafruit_SSD1306/blob/master/Adafruit_SSD1306.cpp
https://www.instructables.com/ESP8266-01-With-Multiple-I2C-Devices-Exploring-ESP/
https://github.com/adafruit/RTClib
[Power Modes](https://blog.creations.de/?p=149)
[Power Consumption](https://bbs.espressif.com/viewtopic.php?t=133)
[Low Power Demo](https://github.com/esp8266/Arduino/tree/master/libraries/esp8266/examples/LowPowerDemo)
[Sleep WakeUp Cause ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#checking-sleep-wakeup-cause)
[Espressif Low Power Solutions](https://www.espressif.com/sites/default/files/9b-esp8266-low_power_solutions_en_0.pdf)
[ATTINY85 Speedometer](https://easyeda.com/sergiu.stanciu/oled_display_attiny)
[Tiny USB-C LiPo Charger](https://www.tindie.com/products/beastdevices/ant-usb-c-lipo-battery-charger/)
[Watchy Hardware](https://watchy.sqfmi.com/docs/hardware/)