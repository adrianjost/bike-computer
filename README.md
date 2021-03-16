# Bike-Computer

## Plannes Features

- [ ] current speed
- [ ] current time
- [ ] max speed
- [ ] total distance
- [ ] current trip
  - [ ] distance
  - [ ] driving time
  - [ ] avg speed
- [ ] wifi data export of all trips
- [ ] auto-trip detection - save current trip on pause (10min timeout?)

### Optional Features

- [ ] "easy" software update
- [ ] stopwatch

## Open Challenges

- [ ] low battery warning
- [x] [self-power-off-circuit](./challenges/self-power-off-circuit/self-power-off-circuit.md)
- [ ] measure speed with Attiny45
- [ ] Attiny45 i2c communication with ESP8266
- [ ] Hardware Debounce Reed-Switch
- [ ] Attiny45 Watchdog Sleep for given time (but continue speed detection)
- [ ] activate WiFi only on certain pages
- [ ] store trips on some kind of storage
- [ ] implement API to download stories from device

## Usefull Links / Tutorials / Resources

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
[Online Plot Maker](https://chart-studio.plotly.com/create/#/)
[JST SH 1mm Connector](https://www.youtube.com/watch?v=wn3ixZ-sv5w)
