#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
RTCDateTime dt;

void setup() {
  // put your setup code here, to run once:
  clock.begin();
  clock.setDateTime(__DATE__, __TIME__);

}

void loop() {

}
