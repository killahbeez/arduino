volatile uint32_t cnt1 = 0;
uint16_t freq_multiplex;

typedef struct buzzer_s {
  uint8_t stage = 0;
  int16_t cnt = 1;
  boolean tick = 0;
} buzzer_t;

volatile buzzer_t buzzer_vars;

void setup() {
  Serial.begin(9600);
  freq_multiplex = 2000;

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);

  DDRD |= (1 << PD3);

  DDRD &= ~(1<<PD4);
  PORTD |= (1<<PD4);

  TCCR2B = (TCCR2B & 0b11111000) | 0b00000010; // set prescaller clk/8 (3.92 kHz)

}

void loop() {
  if((PIND & (1<<PD4)) > 0){
    TCCR2A = ( TCCR2A & 0b11001111 ) | 0b00100000; //enable PWM for PD3
    buzzer();
  }
  else{
    buzzer_vars.cnt = 1;
    buzzer_vars.stage = 0;
    TCCR2A = TCCR2A & 0b11001111; //disable PWM for PD3
  }

  
}

void buzzer() {
  if (buzzer_vars.tick == 1) {
    if (buzzer_vars.stage == 0) {
      if (buzzer_vars.cnt < 255) {
        buzzer_vars.cnt += 30;
        if(buzzer_vars.cnt > 255){
          buzzer_vars.cnt = 255;
        }
        OCR2B = buzzer_vars.cnt;
      }
      else {
        buzzer_vars.stage = 1;
        buzzer_vars.cnt = 0;
      }
    }

    if(buzzer_vars.stage == 1){
      if(buzzer_vars.cnt < 5){
        buzzer_vars.cnt++;
      }
      else{
        buzzer_vars.stage = 2;
        buzzer_vars.cnt = 255;
      }
    }
    if(buzzer_vars.stage == 2){
      if(buzzer_vars.cnt > 1 && buzzer_vars.cnt <= 255){
        buzzer_vars.cnt -= 10;
        if( buzzer_vars.cnt < 1  ){
          buzzer_vars.cnt = 1;
        }
        OCR2B = buzzer_vars.cnt;
      }
      else {
        buzzer_vars.stage = 0;
        buzzer_vars.cnt = 1;
      }
    }

    Serial.println(buzzer_vars.cnt);
    buzzer_vars.tick = 0;
  }
}

ISR(TIMER1_OVF_vect) {
  static uint32_t cnt = 0;

  cnt1++;

  //change buzzer duty cycle every 10ms
  if (cnt1 % (freq_multiplex / 100) == 0) {
    buzzer_vars.tick = 1;
  }

}
