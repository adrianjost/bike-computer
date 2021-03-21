# Data Storage

## Goal

I want to save data from my trips for further analysis. This should include basic informations like total distance and riding time at the start and end of the trip, my speed in given intervals of a trip and eventually how often I stopped. The precise details are open for discussion. I also want to mention, that not all data need to be stored directly. It would be best to use some sort of efficient data structure that can later be exported and processed by other tools to derive the desired values. The values should also survive a complete power down. I don't want to be forced to sync values after every ride.

## Potential Storage Methods

| method  | +                                                         | -                                    |
| ------- | --------------------------------------------------------- | ------------------------------------ |
| SD-Card | can be removed for data analysis, no need for a WiFi Sync | requires lots of IO on uC            |
|         | can be easily exchanged in case of failure                | needs more space                     |
|         | massive capacities available                              | potentially 5-40mA power consumption |
| EEProm  | integrated in uC                                          | limited read/write cycles            |
|         |                                                           | low capacity (0.5-4MB)               |

## Proposed Data Structure

## Usefull Links / Tutorials / Resources

- https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/
- https://simple-circuit.com/arduino-sd-card-interfacing-example/
- [SD-Card Power Consumption](https://forum.arduino.cc/index.php?topic=132303.0)
