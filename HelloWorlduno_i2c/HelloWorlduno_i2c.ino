
#include <Wire.h>
#include <LiquidTWI2.h>
#include <DS3231.h>

LiquidTWI2 lcd(0x20);
DS3231 clock;
RTCDateTime dt;

void setup() {
  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008);

  lcd.begin(16, 2);

  lcd.setBacklight(HIGH);
  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
}

void loop() {
  clock.forceConversion();
  
  dt = clock.getDateTime();
  lcd.setCursor(0, 0);
  lcd.print(clock.dateFormat("H:i:s", dt));
  
  lcd.setCursor(10, 0);
  lcd.print(clock.dateFormat("d-M", dt));

  
  
  lcd.setCursor(6, 1);
  lcd.print("Temp: ");
  lcd.setCursor(11, 1);
  lcd.print(clock.readTemperature());
  //delay(100);

}


