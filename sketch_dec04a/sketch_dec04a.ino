#include "SoftwareSerial.h"

SoftwareSerial Serial_1(10,11);
char didi;

void setup() {
  Serial_1.begin(9600);
  // put your setup code here, to run once:
  DDRB |= (1<<PB5);
  PORTB &= ~(1<<PB5);
}

void loop() {
  if(Serial_1.available()){
    while(Serial_1.available()>0){
      Serial_1.println((char)Serial_1.read());
    }
    PORTB |= (1<<PB5);
  }

}
