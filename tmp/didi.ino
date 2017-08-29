volatile uint16_t i=0;
void setup() {
  //setting timer 1
  TCCR1A = (1 << WGM11) | (1 << WGM10); // Fast PWM OCR1
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) ; // FCPU/256 prescaler
  TIMSK1 = (1 << OCIE1B) | (1 << TOIE1);
  
  OCR1A = 62500 / 2;
  OCR1B = OCR1A / 2;
  
  DDRB |= (1<<PB2);
}

void loop() {
}


ISR(TIMER1_OVF_vect){
  //Serial.println(TCNT1);
  
   PORTB |= (1<<PB2);
}

ISR(TIMER1_COMPB_vect){
  //Serial.println(TCNT1);
   PORTB &= ~(1<<PB2);
   if(OCR1B < (OCR1A-1000))
   OCR1B += 1000;
   else{
OCR1B = OCR1A / 2;
   }
}
