#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
RTCDateTime dt;

void setup() {
  Serial.begin(9600);
  // put your setup code here, to run once:
  clock.begin();
  clock.setDateTime(__DATE__, __TIME__);
  Serial.println(clock.dateFormat("H:i:s", dt));
}

void loop() {
  // put your main code here, to run repeatedly:

}
