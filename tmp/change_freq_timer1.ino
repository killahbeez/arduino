void setup() {

  DDRB |= 1<<PB1 | 1<<PB2;
  TCCR1A = 1<<COM1A0 | 1<<COM1B1 | 1<<WGM11 | 1<<WGM10;
  TCCR1B = 1<<WGM13 | 1<<WGM12 | 1<<CS12 ;
  OCR1A = 6250;
  OCR1B = OCR1A/4;

}

void loop() {
  // put your main code here, to run repeatedly:

}
