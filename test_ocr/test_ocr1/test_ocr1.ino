
void setup() {
  Serial.begin(9600);
  DDRB = (1 << PB1) | (1 << PB2);
  TCCR1A |= (1 << COM1A1) | (1 << COM1B1);
  OCR1A = 0;
}

boolean didi = 0;
void loop() {
  for (int i = 0; i <= 254; i++) {
    OCR1A++;
    Serial.println(OCR1A);
    delay(50);
  }
  OCR1A = 0;
  delay(2000);
}
