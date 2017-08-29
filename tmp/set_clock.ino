#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
void setup() {
  // put your setup code here, to run once:
    clock.begin();
  clock.setDateTime(__DATE__, __TIME__);

}

void loop() {
  // put your main code here, to run repeatedly:

}
