# Low Power Consumption in On-State

## Goal

The bike computer should support a target ride duration of 6 - 10h with an 1.3W LiPo battery. To achive this, various power saving measures need to be implemented

## Idea

The ESP supports various sleep states. Unfortunately the ESP8266 does not have a sleep state, where I can wake up every interval or whenever I get an external interrupt. Both are supported, but not together. I had the idea to split add an Attiny45 that can trigger an external interrupt every interval, so I can use the Forced Light Sleep to reduce the power consumption.
In addition, the Attiny45 by itself has a very low power consumption and should be using less energy for calculating the speed, than wakeing up the ESP8266 every time the REED-Switch is triggered. You can think of it as an kind of Big-Little-Architecture.
This also raised the question how to communicte between the two IC's. I plan to use I2C, because the RTC and OLED Screen are already using the I2C protocoll so I don't need any additional pins.

## Usefull Links / Tutorials / Resources

[Power Modes](https://blog.creations.de/?p=149)
[Power Consumption](https://bbs.espressif.com/viewtopic.php?t=133)
[Low Power Demo](https://github.com/esp8266/Arduino/tree/master/libraries/esp8266/examples/LowPowerDemo)
[Sleep WakeUp Cause ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/sleep_modes.html#checking-sleep-wakeup-cause)
[Espressif Low Power Solutions](https://www.espressif.com/sites/default/files/9b-esp8266-low_power_solutions_en_0.pdf)
[Low Power Attiny45 i2C Sensor](https://github.com/DzikuVx/attiny_photoresistor_i2c)
