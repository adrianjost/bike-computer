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

Problem with SD-Card: I don't have enough PINs on the ESP8266-03 available, except if I can somehow reuse the RX/TX Pins during runtime.

## Code Examples

### SD-Card

```arduino
/*
  Listfiles

  This example shows how print out the files in a
  directory on a SD card

  The circuit:
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4

  created   Nov 2010
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 2 Feb 2014
  by Scott Fitzgerald

  This example code is in the public domain.

*/
#include <SPI.h>
#include <SD.h>

File root;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(115200);

  Serial.print("Initializing SD card...");

  if (!SD.begin(SS)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  root = SD.open("/");

  printDirectory(root, 0);

  Serial.println("done!");
}

void loop() {
  // nothing happens after setup finishes.
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);
      time_t cr = entry.getCreationTime();
      time_t lw = entry.getLastWrite();
      struct tm * tmstruct = localtime(&cr);
      Serial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}
```

## Proposed Data Structure

## Usefull Links / Tutorials / Resources

> ESP8266 comes with it's own SD Lib and the Arduino Default Lib does not compile!

- https://www.mischianti.org/2019/12/15/how-to-use-sd-card-with-esp8266-and-arduino/
- https://simple-circuit.com/arduino-sd-card-interfacing-example/
- [SD-Card Power Consumption](https://forum.arduino.cc/index.php?topic=132303.0)
- [ESP8266 Pin Functions](https://i2.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266-WeMos-D1-Mini-pinout-gpio-pin.png?quality=100&strip=all&ssl=1)
- [EEPROM Wear Leveling Lib](https://github.com/xoseperez/eeprom_rotate)
