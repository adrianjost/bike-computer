# Speed Measurement

Note: Look for RPM implementations. Those can calculate the rotation duration very precisely so it should work equally well for the low rotation speed we encounter on a bike.

[ATTINY85 Speedometer](https://easyeda.com/sergiu.stanciu/oled_display_attiny)

## Speed Tables

| km/h | m/s     | Frequency in Hz (2m circumference) |
| ---- | ------- | ---------------------------------- |
| 5    | 1.3888  | 0.694444                           |
| 10   | 2.7777  | 1.388888                           |
| 15   | 4.1666  | 2.083333                           |
| 20   | 5.5555  | 2.777777                           |
| 25   | 6.9444  | 3.472222                           |
| 30   | 8.3333  | 4.166666                           |
| 35   | 9.7222  | 4.861111                           |
| 40   | 11.1111 | 5.555555                           |
| 45   | 12.5000 | 6.250000                           |
| 50   | 13.8888 | 6.944444                           |
| 55   | 15.2777 | 7.638888                           |
| 60   | 16.6666 | 8.333333                           |

| Frequency in Hz (2m circumference) | m/s | km/h |
| ---------------------------------- | --- | ---- |
| 0.25                               | 0.5 | 1.8  |
| 0.50                               | 1   | 3.6  |
| 0.75                               | 1.5 | 5.4  |
| 1                                  | 2   | 7.2  |
| 1.25                               | 2.5 | 9.0  |
| 1.5                                | 3   | 10.8 |
| 1.75                               | 3.5 | 12.6 |
| 2                                  | 4   | 14.4 |
| 2.5                                | 5   | 18.0 |
| 3                                  | 6   | 21.6 |
| 4                                  | 8   | 28.8 |
| 5                                  | 10  | 36.0 |
| 6                                  | 12  | 43.2 |
| 7                                  | 14  | 50.4 |
| 8                                  | 16  | 57.6 |
| 9                                  | 18  | 64.8 |
| 10                                 | 20  | 72.0 |

## Usefull Links / Tutorials / Resources

[Online Plot Maker](https://chart-studio.plotly.com/create/#/) |
