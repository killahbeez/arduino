
#include <Buttons.h>
#include <SoftwareSerial.h>
SoftwareSerial mySerial(10, 11); // RX, TX
boolean didi = 0;
uint8_t cnt = 0;

Buttons buton_1(14); // PC0
uint16_t freq_multiplex;

volatile uint32_t cnt1 = 0;

void setup() {
  Serial.begin(115200);
  mySerial.begin(115200);

  freq_multiplex = 1000;
  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);

  PORTC |= (1 << PC0);
  DDRB |= (1 << PB5);
  PORTB &= ~(1 << PB5);
}


void loop() {
  static boolean sr = false;
  
  if (buton_1.isClicked()) {
     Serial.write(cnt++);
  }

  while (Serial.available() > 0) {
    mySerial.write(Serial.read());
    sr = true;
  }
  if(sr){
    mySerial.println();
    sr = false;
  }
  delay(10);
  // put your main code here, to run repeatedly:

}

ISR(TIMER1_OVF_vect) {
  cnt1++;

  //check debounce every 5ms
  if ((cnt1 % (int)(0.005 / (1.0 / (float)freq_multiplex))) == 0) {
    buton_1.Debounce();
  }
}
