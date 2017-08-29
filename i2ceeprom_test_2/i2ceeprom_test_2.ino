//Test extEEPROM library.
//Writes the EEPROM full of 32-bit integers and reads them back to verify.
//Wire a button from digital pin 6 to ground, this is used as a start button
//so the sketch doesn't do unnecessary EEPROM writes every time it's reset.
//Jack Christensen 09Jul2014

#include <extEEPROM.h>    //http://github.com/JChristensen/extEEPROM/tree/dev
#include <Streaming.h>    //http://arduiniana.org/libraries/streaming/
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire
#define countArr(number) sizeof(number)/sizeof(number[0])

//Two 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 16;         //for read and write test functions
extEEPROM eep(kbits_16, 1, 16);         //device size, number of devices, page size

byte data[] = "muie la dezinte";


void setup(void)
{
  Serial.begin(9600);
  
  uint8_t eepStatus = eep.begin(twiClock400kHz);      //go fast!
  if (eepStatus) {
    while (1);
  }
  //eep.write(0x00, data, 16);
}

void loop(void)
{
  dumping(0,16);
  delay(1000);
}

void dumping(uint32_t startAddr, uint32_t nBytes)
{
  
  //Serial << endl << F("EEPROM DUMP 0x") << _HEX(startAddr) << F(" 0x") << _HEX(nBytes) << ' ' << startAddr << ' ' << nBytes << endl;

  static uint16_t buff = 0;
  for (uint32_t r = startAddr; r < (startAddr + nBytes); r++) {
    buff = eep.read(r);
    //Serial.print(buff,HEX);
    //Serial.print(" ");
    Serial << _HEX(buff) << ' ';
    Serial << endl;
  }
  
}

