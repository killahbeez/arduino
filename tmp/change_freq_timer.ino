

void setup() {
  DDRB |= (1 << PB1) | (1<< PB2);

  TCCR1A = _BV(COM1A1)  | _BV(COM1B1) | _BV(WGM11)  ; //non-inverted
  //TCCR1A = _BV(COM1A1)  | _BV(COM1B1) | _BV(COM1A0)  | _BV(COM1B0) | _BV(WGM11)  ; //inverted
  
  TCCR1B =   _BV(WGM13) | _BV(WGM12) |_BV(CS12) ;
  ICR1 = 6250;
  OCR1A = 3000;
  //OCR1B = 3000;
  
  // enable timer compare interrupt
  //TIMSK1 |= (1 << OCIE1A);
  //TIMSK1 |= (1 << OCIE1B);
  //Serial.begin(115200);
}

void loop() {
  
  
}
/*
ISR(TIMER1_COMPA_vect) { 
  Serial.println(TCNT1);
}


ISR(TIMER1_COMPB_vect) { 
  Serial.println(TCNT1);
}
*/
