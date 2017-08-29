
#include <TinyWireM.h>
#define Wire TinyWireM
#include <LiquidTWI2.h>

LiquidTWI2 lcd(0x20);


int a =0;
int b =0;
int c =0;
int a1 =0;
int b1 =0;
int c2 =0;
int a3 =0;
int b4 =0;
int c5=0;

void setup()
{
  Wire.begin();

  lcd.setMCPType(LTI_TYPE_MCP23008); 
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);

}


void loop()
{
  byte error, address;
  int nDevices;
lcd.setCursor(0,0);
lcd.clear();
  nDevices = 0;
  for (address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0)
    {


      if (address < 16)
        lcd.print("0");
      lcd.print(address, HEX);
      
      lcd.print(" ");

      nDevices++;
    }
    else if (error == 4)
    {
      lcd.print("Unknow error at address 0x");
      if (address < 16)
        lcd.print("0");
      lcd.print(address, HEX);
      
      lcd.print(" ");
    }
  }
  if (nDevices == 0)
    lcd.print("No I2C devices found\n");
  else
    lcd.print("done\n");

  delay(6000);           // wait 5 seconds for next scan
}
