#include <util/atomic.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(48, 6, NEO_GRB + NEO_KHZ800);

#define BUTTON_MODE PB0
#define BUTTON_DIM PD7
#define countArr(number) sizeof(number)/sizeof(number[0])

//MSGEQ7 pins
#define channelPin 0
#define strobePin PB4
#define resetPin PB5

uint8_t pressed[] = {0, 0};
uint8_t released[] = {1, 1};
uint32_t millis_pressed[] = {0, 0};
volatile boolean isTurnedON = 1;

uint16_t spectrumLeftValue[7];
uint16_t spectrumRightValue[7];
uint8_t filter = 120;
uint16_t spectrumValue[7];
uint16_t amplitudeBitsLast[7];
uint16_t maxFreq;
uint8_t modeType = 1;
uint16_t speedFall = 80;

uint32_t last_millis[7];
uint32_t last_millis_1 = 0;
uint8_t brightness = 0;

uint32_t colors[8];
boolean setColors = 1;

void setup() {

  //Serial.begin(9600);
  DDRB &= ~(1 << BUTTON_MODE); //BUTTON_MODE INPUT
  PORTB |= (1 << BUTTON_MODE); //BUTTON_MODE INPUT_PULLUP

  DDRD &= ~(1 << BUTTON_DIM); //BUTTON_DIM INPUT
  PORTD |= (1 << BUTTON_DIM); //BUTTON_DIM INPUT_PULLUP

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 1250; //200Hz
  TIMSK1 = (1 << TOIE1);

  DDRB |= (1 << strobePin) | (1 << resetPin);

  last_millis_1 = millis();

  MSGEQ7_reset();

  pixels.begin();
  pixels.setBrightness(brightness * 50 + 2);
  
  uint32_t colors[] = {
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0),
    pixels.Color(255, 0, 0)
  };
}

void loop() {
  checkSwitchOFF(); //if not released button 1 for more than 3 sec lights turn off

  if (buttonDim()) {
    pressed[0] = 0;
    ++brightness;
    brightness %= 5;
    pixels.setBrightness(brightness * 50 + 2);
  }

  if (buttonMode()) {
    setColors = 1;
    ++modeType %= 2;
    pressed[1] = 0;
  }

  if (setColors) {
    switch (modeType) {
      case 0:
        colors[0] = pixels.Color(0, 255, 0);
        colors[1] = pixels.Color(0, 255, 0);
        colors[2] = pixels.Color(0, 255, 0);
        colors[3] = pixels.Color(0, 255, 0);
        colors[4] = pixels.Color(0, 255, 0);
        colors[5] = pixels.Color(0, 255, 0);
        colors[6] = pixels.Color(0, 255, 0);
        colors[7] = pixels.Color(0, 255, 0);
        break;
      case 1:
        colors[0] = pixels.Color(0, 0, 255);
        colors[1] = pixels.Color(0, 0, 255);
        colors[2] = pixels.Color(255, 255, 0);
        colors[3] = pixels.Color(255, 255, 0);
        colors[4] = pixels.Color(255, 255, 0);
        colors[5] = pixels.Color(255, 0, 0);
        colors[6] = pixels.Color(255, 0, 0);
        colors[7] = pixels.Color(255, 0, 0);
        break;
      default:
        colors[0] = pixels.Color(255, 0, 0);
        colors[1] = pixels.Color(255, 0, 0);
        colors[2] = pixels.Color(255, 0, 0);
        colors[3] = pixels.Color(255, 0, 0);
        colors[4] = pixels.Color(255, 0, 0);
        colors[5] = pixels.Color(255, 0, 0);
        colors[6] = pixels.Color(255, 0, 0);
        colors[7] = pixels.Color(255, 0, 0);
    };
    setColors = 0;
  }

  if (isTurnedON) {
    MSGEQ7_read();
    
    pixels.clear();
    pixels.show();
    ledLights(spectrumValue, countArr(spectrumValue), modeType);
    pixels.show();
  }
}

void ledLights(uint16_t *spectrumValue, uint8_t sizeSpectrum, uint8_t type) {
  uint8_t amplitudeBits = 0;
  uint8_t max_amplitudeBits = 0;

  //MODE 0 :: lights all leds till amplitude
  if (type == 0) {
    for (int i = 0 ; i < 6; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }
      lightBand(i, amplitudeBits, colors);
    }
  }

  //MODE 1 :: center maxim frequency and distribute the rest
  if (type == 1) {
    max_amplitudeBits = floor(maxFreq / 128) + 1;
    for (int i = 0 ; i < 6; i++) {
      if (maxFreq < filter) {
        max_amplitudeBits = 0;
      }

      //if is center
      if (i == 3) {
        lightBand(i, max_amplitudeBits, colors);
      }
      else if (i < 3) {
        lightBand(i, (
                    (max_amplitudeBits - (3 - i)) > 0
                    ?
                    (max_amplitudeBits - (3 - i))
                    :
                    0
                  ), colors);
      }
      else {
        lightBand(i, (
                    (max_amplitudeBits - (i - 3)) > 0
                    ?
                    (max_amplitudeBits - (i - 3))
                    :
                    0
                  ), colors);
      }
    }
  }
}

void lightBand(uint8_t col, uint8_t amplitude, uint32_t *colors) {
  uint8_t a = 0;

  for (uint8_t i = 8 * col; i < (8 * col + amplitude); i++) {
    pixels.setPixelColor(i, colors[a++]);
  }
}

void MSGEQ7_reset() {
  PORTB &= ~(1 << resetPin);  //init with STROBE and RESET low
  PORTB &= ~(1 << strobePin);

  PORTB |= (1 << resetPin) ; //set RESET high
  PORTB |= (1 << strobePin); //set STROBE high
  delayMicroseconds(18); //delay 18us for STROBE high (see timing diagram)

  PORTB &= ~(1 << strobePin); //set STROBE low
  delayMicroseconds(54); //delay 54us (tss 72us) for STROBE low (see timing diagram)

  PORTB &= ~(1 << resetPin); //set RESET low
  PORTB |= (1 << strobePin); //set STROBE high
  delayMicroseconds(18); //delay 18us for STROBE high (see timing diagram)
}

void MSGEQ7_read() {
  maxFreq = 0;

  for (int8_t i = 0; i <= 6; i++) {
    PORTB &= ~(1 << strobePin); //set STROBE low
    delayMicroseconds(10); // to = 36 us (see timing diagram)
    spectrumValue[i] = analogRead(channelPin);

    if (spectrumValue[i] < filter) {
      spectrumValue[i] = 0;
    }

    if (modeType == 1) { //get maximum value of frequencies
      if (i == 0) {
        maxFreq = spectrumValue[i];
      }
      else if (spectrumValue[i] > maxFreq && i <= 2) {
        maxFreq = spectrumValue[i];
      }
    }

    PORTB |= (1 << strobePin); //set STROBE high
    delayMicroseconds(10); // tss = 72 us (see timing diagram)
  }


}

ISR(TIMER1_OVF_vect) {
  //every 5 ms check for debounce
  debouncePress();
}

void debouncePress() {

  volatile static uint8_t pins[] = {PD7, PB0};
  volatile uint8_t pinb_arr[] = {PIND, PINB};
  volatile static uint8_t isPushed[] = {0, 0};
  volatile static uint8_t lastState[] = {0, 0};
  volatile static uint8_t count[] = {0, 0};

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
      pressed[i] = 0;
      released[i] = 1;
      count[i] = 0;
    }
  }

}

boolean buttonPressed(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (pressed[pin]) {
      isTurnedON = 1;
      return true;
    }

    return false;
  }
}

boolean buttonReleased(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (released[pin]) {
      return true;
    }

    return false;
  }
}

boolean buttonDim() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (buttonPressed(0)) {
      return true;
    }

    return false;
  }
}

boolean buttonMode() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (buttonPressed(1)) {
      return true;
    }

    return false;
  }
}

boolean checkSwitchOFF() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (!buttonReleased(0)) {
      if ((millis() - millis_pressed[0]) >= 1000) {
        isTurnedON = 0;
        pixels.clear();
        pixels.show();
        return true;
      }
    }

    return false;
  }
}


