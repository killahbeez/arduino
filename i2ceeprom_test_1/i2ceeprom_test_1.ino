//Test extEEPROM library.
//Writes the EEPROM full of 32-bit integers and reads them back to verify.
//Wire a button from digital pin 6 to ground, this is used as a start button
//so the sketch doesn't do unnecessary EEPROM writes every time it's reset.
//Jack Christensen 09Jul2014

#include <extEEPROM.h>    //http://github.com/JChristensen/extEEPROM/tree/dev
#include <Streaming.h>    //http://arduiniana.org/libraries/streaming/
#include <Wire.h>         //http://arduino.cc/en/Reference/Wire

//Two 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 2;         //for read and write test functions
extEEPROM eep(kbits_16, 1, 16);         //device size, number of devices, page size

byte data[] = "muie";
byte dataqq1[] = "muie la dezdsdsdsinte";
byte datwwa2[] = "muie la dezisdadsnte";
byte dqwwata3[] = "muie la dezidsdsdsdsnte";



void setup(void)
{
  Serial.begin(9600);
  //twiClock400kHz
  uint8_t eepStatus = eep.begin();      //go fast!
  if (eepStatus) {
    //Serial << endl << F("extEEPROM.begin() failed, status = ") << eepStatus << endl;
    while (1);
  }

  //Serial.println(countArr(data));
 // Serial.println(countArr(data));
  //Serial.println("______________");
  uint8_t chunkSize = 64;
  //eeErase(chunkSize, 0, totalKBytes * 1024 - 1);
  //eep.write(0x00, data, 5);
  //dumping(0, 6);            //the first 1024 bytes
}

void loop(void)
{
  
  dumping(0, 4);            //the first 1024 bytes
  delay(100);
}

//dump eeprom contents, 16 bytes at a time.
//always dumps a multiple of 16 bytes.
void dumping(uint32_t startAddr, uint32_t nBytes)
{
  
  //Serial << endl << F("EEPROM DUMP 0x") << _HEX(startAddr) << F(" 0x") << _HEX(nBytes) << ' ' << startAddr << ' ' << nBytes << endl;

  static uint16_t buff = 0;
  for (uint32_t r = startAddr; r < (startAddr + nBytes); r++) {
    buff = eep.read(r);
    //Serial.println("didi");
    //Serial.println(buff,HEX);
    Serial << _HEX(buff) << " : " ;
  }
  
}

//write 0xFF to eeprom, "chunk" bytes at a time
void eeErase(uint8_t chunk, uint32_t startAddr, uint32_t endAddr)
{
    chunk &= 0xFC;                //force chunk to be a multiple of 4
    uint8_t data[chunk];
    Serial << F("Erasing...") << endl;
    for (int i = 0; i < chunk; i++) data[i] = 0xFF;
    uint32_t msStart = millis();
    
    for (uint32_t a = startAddr; a <= endAddr; a += chunk) {
        if ( (a &0xFFF) == 0 ) Serial << a << endl;
        eep.write(a, data, chunk);
    }
    uint32_t msLapse = millis() - msStart;
    Serial << "Erase lapse: " << msLapse << " ms" << endl;
}


/*
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
*/

