#include <TinyWireM.h>
#include <LiquidTWI2.h>
#define Wire TinyWireM

LiquidTWI2 lcd(0x20);
uint8_t dexter;

void setup() {

  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008);
  lcd.begin(16, 2);
  lcd.setBacklight(HIGH);
  delay(100);

  Wire.begin(); 

}

void loop() {

  didi();
  lcd.setCursor(6, 1);
  lcd.print("Temp: ");
  lcd.setCursor(11, 1);
  float temp = readTemperature();
  lcd.print(temp);
  delay(500);

}


uint8_t didi(void) {

  char sec_buff[3];
  uint8_t sec;

  Wire.beginTransmission(0x68);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.requestFrom(0x68, 1);

  while (Wire.available()) {

    sec = bcd2dec(Wire.read());
    lcd.clear();
    lcd.setCursor(0, 0);
    sprintf(sec_buff, "%02d", sec);

    lcd.print(sec_buff);
  }
  Wire.endTransmission();
  return 1;
}

float readTemperature(void)
{
  uint8_t msb, lsb;

  Wire.beginTransmission(0x68);
  Wire.write(0x11);
  Wire.endTransmission();

  Wire.requestFrom(0x68, 2);

  while (Wire.available()) {
    msb = Wire.read();
    lsb = Wire.read();
  }

  return (((short)msb << 2) | ((short)lsb >> 6)) / 4.0f;
}

uint8_t bcd2dec(uint8_t bcd)
{
  return ((bcd / 16) * 10) + (bcd % 16);
}




