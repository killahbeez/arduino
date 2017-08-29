//Test extEEPROM library.
//Writes the EEPROM full of 32-bit integers and reads them back to verify.
//Wire a button from digital pin 6 to ground, this is used as a start button
//so the sketch doesn't do unnecessary EEPROM writes every time it's reset.
//Jack Christensen 09Jul2014

#include <extEEPROM.h>    //http://github.com/JChristensen/extEEPROM/tree/dev
#include <Streaming.h>    //http://arduiniana.org/libraries/streaming/
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire

//Two 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 16;         //for read and write test functions
extEEPROM eep(kbits_16, 1, 16);         //device size, number of devices, page size

byte data[4] = {'m', 'u', 'i', 'e'};

void setup(void)
{
  Serial.begin(9600);
  
  uint8_t eepStatus = eep.begin();      //go fast!
  if (eepStatus) {
    Serial << endl << F("extEEPROM.begin() failed, status = ") << eepStatus << endl;
    while (1);
  }

  uint8_t chunkSize = 4;
  eep.write(0x01, data, 4);
  dump(0, 16);            //the first 1024 bytes
}

void loop(void)
{
}

//dump eeprom contents, 16 bytes at a time.
//always dumps a multiple of 16 bytes.
void dump(uint32_t startAddr, uint32_t nBytes)
{
  Serial << endl << F("EEPROM DUMP 0x") << _HEX(startAddr) << F(" 0x") << _HEX(nBytes) << ' ' << startAddr << ' ' << nBytes << endl;
  uint32_t nRows = (nBytes + 15) >> 4;

  uint8_t d[16];
  for (uint32_t r = 0; r < nRows; r++) {
    uint32_t a = startAddr + 16 * r;
    eep.read(a, d, 16);
    Serial << "0x";
    if ( a < 16 * 16 * 16 ) Serial << '0';
    if ( a < 16 * 16 ) Serial << '0';
    if ( a < 16 ) Serial << '0';
    Serial << _HEX(a) << ' ';
    for ( int c = 0; c < 16; c++ ) {
      if ( d[c] < 16 ) Serial << '0';
      Serial << _HEX( d[c] ) << ( c == 7 ? "  " : " " );
    }
    Serial << endl;
  }
}

