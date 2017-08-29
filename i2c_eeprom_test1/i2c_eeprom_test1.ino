// i2c_scanner
//
// Version 1
//    This program (or code that looks like it)
//    can be found in many places.
//    For example on the Arduino.cc forum.
//    The original author is not know.
// Version 2, Juni 2012, Using Arduino 1.0.1
//     Adapted to be as simple as possible by Arduino.cc user Krodal
// Version 3, Feb 26  2013
//    V3 by louarnold
// Version 4, March 3, 2013, Using Arduino 1.0.3
//    by Arduino.cc user Krodal.
//    Changes by louarnold removed.
//    Scanning addresses changed from 0...127 to 1...119,
//    according to the i2c scanner by Nick Gammon
//    http://www.gammon.com.au/forum/?id=10896
// Version 5, March 28, 2013
//    As version 4, but address scans now to 127.
//    A sensor seems to use address 120.
// 
//
// This sketch tests the standard 7-bit addresses
// Devices with higher bit address might not be seen properly.
//

#include <Wire.h>
#define countArr(number) sizeof(number)/sizeof(number[0])

  byte error;

byte address[] = {0b1010000,0b1000000};

void setup()
{
  Wire.begin();

  Serial.begin(9600);
  Serial.println("\nI2C Scanner");
}


void loop()
{

  Serial.println("Scanning...");
  for(uint8_t i = 0;i<countArr(address);i++){
    Wire.beginTransmission(address[i]);
    error = Wire.endTransmission();

    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address[i]<16) 
        Serial.print("0");
      Serial.print(address[i],HEX);
      Serial.println("  !");

    }
    else
    {
      Serial.print("Unknow error at address 0x");
      if (address[i]<16) 
        Serial.print("0");
      Serial.println(address[i],HEX);
    }    
  }

  delay(5000);           // wait 5 seconds for next scan
}

