
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX
boolean didi = 0;
uint8_t cnt = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);
  PORTC |= (1 << PC0);
  DDRB |= (1 << PB5);
  PORTB &= ~(1 << PB5);
}

void loop() {
  if ((PINC & (1 << PC0)) == 0) {
    PORTB |= (1 << PB5);
    if (!didi) {
      didi = 1;
      Serial.print(cnt++);
    }
    delay(100);
  }
  else if (didi) {

    PORTB &= ~(1 << PB5);
    didi = 0;
  }

  while (Serial.available() > 0) {
    mySerial.write(Serial.read());
  }
  // put your main code here, to run repeatedly:

}
