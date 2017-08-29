uint32_t last_millis;
uint32_t didi;
 
void setup() {
  DDRB = (1<<PB5);
  //TCCR0A = (1<<WGM01) | (1<<WGM00);
  //TCCR0B = (1<<WGM02) | (1<<CS01) | (1<<CS00);
  //OCR0A = 25;
}

void loop() {
  PORTB ^= (1<<PB5);
  
  delay(10000);
}

