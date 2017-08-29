#include <Time.h>

uint8_t a = 0;
uint8_t c1 = 0;
uint8_t c3 = 0;
uint8_t c11 = 0;
uint8_t c32 = 0;
time_t t;
void setup() {
  t = now();
  setTime(t);
  Serial.begin(9600);

}

void loop() {
uint8_t b = 0;
uint8_t c = 0;
  Serial.println(second(t));

}
