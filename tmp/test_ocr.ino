void setup() {
  TCCR0A = (1<<COM0A1) | (1<<COM0B1) | (1<<WGM01) | (1<<WGM00);
  TCCR0B = (1<<CS00);
  OCR0A = 10;
  OCR0B = 130;
  DDRD |= (1<<PD5) | (1<<PD6);
}

void loop() {

}
