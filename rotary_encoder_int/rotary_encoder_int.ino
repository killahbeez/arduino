/*
 * GND -> GND
 * + -> 5v
 * SW -> PD4
 * DT -> A0
 * CLK -> A1 
 */

#include <util/atomic.h>
#include <Buttons.h>

Buttons button_1(4);

#define ENC_CTL  DDRC  //encoder port control
#define ENC_WR  PORTC //encoder port write  
#define ENC_RD  PINC  //encoder port read

uint8_t pressed[] = {0};
uint8_t released[] = {1};
uint32_t millis_pressed[] = {0};
uint32_t millis_released[] = {0};

volatile int8_t x = 0;


void setup() {
  Serial.begin(9600);
  PCMSK1 |= (( 1 << PCINT8 ) | ( 1 << PCINT9 )); //enable encoder pins interrupt sources
  /* enable pin change interupts */
  PCICR |= ( 1 << PCIE1 );

  TIMSK1 = (1 << TOIE1);


}
boolean first = false;
uint32_t time_pressed = 0;

void loop() {
  
  if (button_1.isClicked()) {
    Serial.println("pressed");
  }
  
}


/* encoder routine. Expects encoder with four state changes between detents */
/* and both pins open on detent */
ISR(PCINT1_vect)
{
  static uint8_t old_AB = 3;  //lookup table index
  static int8_t encval = 0;   //encoder value
  static const int8_t enc_states [] PROGMEM = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; //encoder lookup table

  old_AB <<= 2; //remember previous state
  old_AB |= ( ENC_RD & 0x03 );
  encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));
  
  //on detent
  if ( encval > 3 ) { //four steps forward
    ++x;
    Serial.println(x);
    encval = 0;
  }
  else if ( encval < -3 ) { //four steps backwards
    --x;
    Serial.println(x);
    encval = 0;
  }

}

ISR(TIMER1_OVF_vect) {
  static uint32_t cnt = 0;
  static uint32_t delay_button = (int)(0.005 / (1.0 / (float)490));

  cnt++;

  //check debounce
  if ( (cnt % delay_button) == 0) {
    button_1.Debounce();
  }

}
