#define CLK PD2
#define DATA PD3
#define LATCH PD4

void setup() {
  DDRD |= (1 << CLK) | (1 << DATA) | (1 << LATCH);
}

void loop() {
  /*static int8_t cnt = 15;
  PORTD &= ~(1<<LATCH);
  for(uint8_t i = 0;i<16;i++){
    PORTD &= ~(1<<CLK);
    PORTD &= ~(1<<DATA);
    if(i == cnt) PORTD |= (1<<DATA);

    PORTD |= (1<<CLK);
  }
  delay(100);
  cnt--;
  if(cnt < 0) {
    cnt = 15;
  }
  PORTD |= (1 << LATCH);
  */
  
  PORTD &= ~(1<<LATCH);
  shiftOut(DATA, CLK, LSBFIRST, 0b11111111);
  shiftOut(DATA, CLK, LSBFIRST, 0b11111111);
  PORTD |= (1 << LATCH);
  while (1);

}
