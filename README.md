# Bike-Computer

## Development Notes

### Pin Compatibility

| Arduino Pin | IN  | OUT |  SDA | SCK | note                                                                                   |
| :---------: | :-: | :-: | :--: | :-: | -------------------------------------------------------------------------------------- |
|      0      | ✅  | ✅  |  ❌  | ✅  | is pulled high during normal operation, PULL-UP is best practice                       |
|      1      | ✅  | ✅  |  ❌  | ❌  | input seems to have internal PULL-UP (needs testing, if true, add external to be safe) |
|      2      | ✅  | ✅  |  ✅  | ❌  | can’t be low at boot                                                                   |
|      3      | ✅  | ✅  |  ❌  | ❌  |                                                                                        |

![ESP8266-01 Pinout & Boot Modes](https://i.pinimg.com/originals/cf/7e/8d/cf7e8de45255400203f46996d8af9603.png)
