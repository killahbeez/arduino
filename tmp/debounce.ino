#include <util/atomic.h>

uint8_t cntP = 0;
uint8_t cntR = 0;

volatile uint8_t pressed[] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t released[] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t cntPP[] = {0, 0, 0, 0, 0, 0, 0, 0};
volatile uint8_t cntRR[] = {0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  Serial.begin(9600);
  TCCR2A |= (1 << WGM20) | (1 << WGM21);
  TCCR2B  |= (1 << CS22) | (1 << CS21) | (1 << CS20) | (1 << WGM22); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR2A = 78; // 200Hz (5ms period)
  TIMSK2 |= (1 << TOIE2);

  DDRB &= ~(1 << PB0); //set PB0 as INPUT
  PORTB |= (1 << PB0); //set INPUT PULLUP resistor

  DDRB &= ~(1 << PB1); //set PB0 as INPUT
  PORTB |= (1 << PB1); //set INPUT PULLUP resistor
}

void loop() {
  buttonPressed(0);
  buttonPressed(1);
  buttonReleased(0);
  buttonReleased(1);
}

void debouncePress() {
  volatile static uint8_t pins[] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    if (!pressed[i]) {
      isPushed[i] = ( ( PINB & (1 << pins[i]) ) == 0);

      //check if button was pressed
      if (isPushed[i] && !lastState[i]) { // if pressed

        count[i]++;

        // if button has not bounce for 4 checks, the button is debounce and considered pressed
        if (count[i] >= 4) {
          lastState[i] = isPushed[i];
          pressed[i] = 1;
          count[i] = 0;
        }
      }
      else if (!isPushed[i] && lastState[i]) { //if released
        lastState[i] = isPushed[i];
        count[i] = 0;
      }
    }
  }
}


void debounceRelease() {
  volatile static uint8_t pins[] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    if (!released[i]) {
      isPushed[i] = ( ( PINB & (1 << pins[i]) ) == 0);

      //check if button was released
      if (!isPushed[i] && lastState[i]) { // if released

        count[i]++;

        // if button has not bounce for 4 checks, the button is debounce and considered released
        if (count[i] >= 4) {
          lastState[i] = isPushed[i];
          released[i] = 1;
          count[i] = 0;
        }
      }
      else if (isPushed[i] && !lastState[i]) { //if pressed
        lastState[i] = isPushed[i];
        count[i] = 0;
      }
    }
  }
}


boolean buttonPressed(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (pressed[pin]) {
      cntPP[pin]++;
      pressed[pin] = 0;
      Serial.print("Pressed ");
      Serial.print(pin);
      Serial.print(": ");
      Serial.println(cntPP[pin]);
    }
  }
}

boolean buttonReleased(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (released[pin]) {
      cntRR[pin]++;
      released[pin] = 0;
      Serial.print("Releases ");
      Serial.print(pin);
      Serial.print(": ");
      Serial.println(cntRR[pin]);
    }
  }
}

ISR(TIMER2_OVF_vect) {
  debouncePress();
  debounceRelease();
}


/*

void debouncePR() {
  volatile static uint8_t pins[] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t lastStateP[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t lastStateR[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t countP[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t countR[] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    if (!pressed[i]) {
      isPushed[i] = ( ( PINB & (1 << pins[i]) ) == 0);

      //check if button was pressed
      if (isPushed[i] && !lastStateP[i]) { // if pressed

        countP[i]++;

        // if button has not bounce for 4 checks, the button is debounce and considered pressed
        if (countP[i] >= 4) {
          lastStateP[i] = isPushed[i];
          pressed[i] = 1;
          countP[i] = 0;
        }
      }
      else if (!isPushed[i] && lastStateP[i]) { //if released
        lastStateP[i] = isPushed[i];
        countP[i] = 0;
      }
    }


    if (!released[i]) {
      isPushed[i] = ( ( PINB & (1 << pins[i]) ) == 0);

      //check if button was released
      if (!isPushed[i] && lastStateR[i]) { // if released

        countR[i]++;

        // if button has not bounce for 4 checks, the button is debounce and considered released
        if (countR[i] >= 4) {
          lastStateR[i] = isPushed[i];
          released[i] = 1;
          countR[i] = 0;
        }
      }
      else if (isPushed[i] && !lastStateR[i]) { //if pressed
        lastStateR[i] = isPushed[i];
        countR[i] = 0;
      }
    }


  }

}

boolean buttonReleased(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (released[pin]) {
      cntRR[pin]++;
      released[pin] = 0;
      Serial.print("Releases ");
      Serial.print(pin);
      Serial.print(": ");
      Serial.println(cntRR[pin]);
    }
  }
}
*/
