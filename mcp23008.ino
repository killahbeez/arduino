#include "Wire.h"
#include <MCP23008.h>

#define MCP23008_ADDR 0x40
 
MCP23008 MyMCP(MCP23008_ADDR);


void setup()
{
  Wire.begin();
  Serial.begin(9600);
  Serial.println(MyMCP.getADDR(),HEX);
  MyMCP.writeIODIR(0x00);      
  MyMCP.writeGPIO(0xFF);
}
 
void loop()
{
 
  MyMCP.writeGPIO(0x7f);  // Write inputs to outputs
  Serial.println(MyMCP.readGPIO(),HEX);
  delay(1000);                   // Wait 1/10th second and repeat
  MyMCP.writeGPIO(0xbf);  // Write inputs to outputs
  Serial.println(MyMCP.readGPIO(),HEX);
  delay(1000);                   // Wait 1/10th second and repeat
}
