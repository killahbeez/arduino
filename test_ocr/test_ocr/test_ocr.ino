
void setup() {
  DDRB = (1 << PB1) | (1 << PB2);
  TCCR1A |= (1 << COM1A1) | (1 << COM1B1);
  OCR1A = 255;
  OCR1B = 0 ;
}

boolean didi = 0;
void loop() {
  for (int i = 0; i <= 254; i++) {
    if (!didi) {
      OCR1A--;
      OCR1B++;
    }
    else {
      OCR1A++;
      OCR1B--;

    }
    delay(10);
  }
  didi = !didi;
}
