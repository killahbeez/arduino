#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(8, 6, NEO_GRB + NEO_KHZ800);
uint32_t color;

#include <util/atomic.h>
#define ENC_CTL  DDRC  //encoder port control
#define ENC_WR  PORTC //encoder port write  
#define ENC_RD  PINC  //encoder port read
#define ENC_A 0
#define ENC_B 1

#define BUTTON PC2
uint8_t pressed[] = {0};
uint8_t released[] = {1};
uint32_t millis_pressed[] = {0};
uint32_t millis_released[] = {0};
#define countArr(number) sizeof(number)/sizeof(number[0])

volatile int8_t x = 0;
volatile boolean refresh = true;

void setup() {
  Serial.begin(9600);
  //ENC_WR |= (( 1<<ENC_A )|( 1<<ENC_B ));    //turn on pullups
  PCMSK1 |= (( 1 << PCINT8 ) | ( 1 << PCINT9 )); //enable encoder pins interrupt sources
  /* enable pin change interupts */
  PCICR |= ( 1 << PCIE1 );

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 1250; //200Hz
  TIMSK1 = (1 << TOIE1);

  PORTC |= (1 << BUTTON); //BUTTON INPUT_PULLUP

  pixels.begin();
  pixels.setBrightness(100);
  color = pixels.Color(250, 0, 0);
}

void loop() {
  if (buttonPressed(0)) {
    pressed[0] = 0;
    Serial.println("pressed");
  }

  if (refresh) {
    if (x >= 0 && x <= 7) {
      pixels.clear();
      pixels.setPixelColor(x, color);
      pixels.show();
    }
    refresh = false;
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
    //if (x < 7) 
    ++x;
    Serial.println(x);
    refresh = true;
    encval = 0;
  }
  else if ( encval < -3 ) { //four steps backwards
    //if (x > 0)
    --x;
    Serial.println(x);
    refresh = true;
    encval = 0;
  }
}

ISR(TIMER1_OVF_vect) {
  //every 5 ms check for debounce
  debouncePress();
}

void debouncePress() {

  volatile static uint8_t pins[] = {BUTTON};
  volatile uint8_t pinb_arr[] = {PINC};
  volatile static uint8_t isPushed[] = {0};
  volatile static uint8_t lastState[] = {0};
  volatile static uint8_t count[] = {0};

  for (uint8_t i = 0; i < countArr(pins); i++) {

    isPushed[i] = ( ( pinb_arr[i] & (1 << pins[i]) ) == 0);

    //check if button was pressed
    if (isPushed[i] && !lastState[i]) { // if pressed
      count[i]++;
      // if button has not bounce for 4 checks, the button is debounce and considered pressed
      if (count[i] >= 4) {
        lastState[i] = isPushed[i];
        pressed[i] = 1;
        released[i] = 0;
        millis_pressed[i] = millis();
        count[i] = 0;
      }
    }
    else if (!isPushed[i] && lastState[i]) { //if released
      lastState[i] = isPushed[i];
      millis_released[i] = millis();
      pressed[i] = 0;
      released[i] = 1;
      count[i] = 0;
    }
  }

}

boolean buttonPressed(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (pressed[pin]) {
      return true;
    }

    return false;
  }
}

