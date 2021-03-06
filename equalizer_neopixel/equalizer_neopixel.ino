#include <util/atomic.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(56, 6, NEO_GRB + NEO_KHZ800);

#define BUTTON_MODE PD2
#define BUTTON_COLORS PD4
#define BUTTON_DIM_UP PB0
#define BUTTON_DIM_DOWN PD7
#define countArr(number) sizeof(number)/sizeof(number[0])
#define NUM_COLS 7

//MSGEQ7 pins
#define channelPin 0
#define strobePin PB4
#define resetPin PB5

uint8_t pressed[] = {0, 0, 0, 0};
uint8_t released[] = {1, 1, 1, 1};
uint32_t millis_pressed[] = {0, 0, 0, 0};
uint32_t millis_released[] = {0, 0, 0, 0};
uint32_t last_millis = 0;
uint32_t last_millis_fall[7];
uint32_t last_millis_stay_max[7];

uint16_t spectrumLeftValue[7];
uint16_t spectrumRightValue[7];
uint8_t filter = 120;
uint16_t spectrumValue[7];
uint16_t amplitudeBitsLast[7];
uint16_t maxFreq;
uint8_t modeType = 1;
uint16_t speedFall = 60;//the speed (ms) of max amplitude falling
uint16_t stayMaxFall = 200; // how much time (ms) will stay at max amplitude before starting to fall
uint16_t speedFall_1 = 40;

uint8_t brightness = 40;
uint8_t max_brightness = 200;

uint32_t colors[8];
uint8_t cntColors = 0;

uint32_t peakColor;

boolean debug = 0;
boolean first_splash = 1;
uint16_t sum_spectrumValues = 0;
boolean debug_spectrumValues = 0;
boolean debug_once = 0;

void setup() {
  DDRD &= ~(1 << BUTTON_MODE); //BUTTON_MODE INPUT
  PORTD |= (1 << BUTTON_MODE); //BUTTON_MODE INPUT_PULLUP

  DDRB &= ~(1 << BUTTON_DIM_UP); //BUTTON_DIM INPUT
  PORTB |= (1 << BUTTON_DIM_UP); //BUTTON_DIM INPUT_PULLUP

  DDRD &= ~(1 << BUTTON_COLORS); //BUTTON_COLORS INPUT
  PORTD |= (1 << BUTTON_COLORS); //BUTTON_COLORS INPUT_PULLUP

  DDRD &= ~(1 << BUTTON_DIM_DOWN); //BUTTON_NAN INPUT
  PORTD |= (1 << BUTTON_DIM_DOWN); //BUTTON_NAN INPUT_PULLUP

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 1250; //200Hz
  TIMSK1 = (1 << TOIE1);

  DDRB |= (1 << strobePin) | (1 << resetPin);

  MSGEQ7_reset();

  pixels.begin();
  pixels.setBrightness(brightness + 1);

  setColors(cntColors);
}

void loop() {
  
  //press MODE for more than 1 sec to enter debug mode
  if (!buttonReleased(0) && (millis() - millis_pressed[0]) > 1000) {
    debug = !debug;
    if (debug) {
      Serial.begin(9600);
      Serial.println("Starting debugging...");
    }
    else {
      Serial.println("Stop debugging...");
      Serial.end();
    }
    pressed[0] = 0;
    released[0] = 1;
  }

  //MODE - cycle throw modes
  if (buttonPressed(0)) {
    ++modeType %= 4;
    pressed[0] = 0;
    if (debug) {
      Serial.print("SET Mode type: ");
      Serial.println(modeType);
    }
  }

  //COLORS - cycle throw colors
  if (buttonPressed(1)) {
    ++cntColors %= 4;
    setColors(cntColors);
    pressed[1] = 0;

    if (debug) {
      Serial.print("SET Colors: ");
      Serial.println(cntColors);
    }
  }

  //DIM - cycle throw brightness UP (DIM)
  if (buttonPressed(3)) {
    if (brightness < max_brightness) {
      brightness = floor(brightness / 10) * 10 + 10;
    }
    pixels.setBrightness(brightness + 1);
    pressed[3] = 0;

    if (debug) {
      Serial.print("DIM UP: ");
      Serial.println(brightness + 1);
    }
  }

  //DIM - cycle throw brightness DOWN (DIM)
  if (buttonPressed(2)) {
    if (brightness > 10) {
      brightness = floor(brightness / 10) * 10  - 10;
    }
    else if (brightness > 0) {
      brightness = 0;
    }
    pixels.setBrightness(brightness + 1);
    pressed[2] = 0;

    if (debug) {
      Serial.print("DIM DOWN: ");
      Serial.println(brightness + 1);
    }
  }

  //if buton DIM UP is pressed and not released for more than 1 sec, it will increment automaticallly the brightness, till max brightness
  if ( !buttonReleased(3)
       &&
       (millis() - millis_pressed[3]) > 1000
       &&
       (millis() - last_millis) > 20
     ) {
    if (brightness < max_brightness) {
      brightness++;
      pixels.setBrightness(brightness + 1);

      if (debug) {
        Serial.print("DIM UP incr: ");
        Serial.println(brightness + 1);
      }
    }
    last_millis = millis();
  }

  //if buton DIM DOWN is pressed and not released for more than 1 sec, it will decrement automaticallly the brightness, till 0
  if ( !buttonReleased(2)
       &&
       (millis() - millis_pressed[2]) > 1000
       &&
       (millis() - last_millis) > 20
     ) {
    if (brightness > 0) {
      brightness--;
      pixels.setBrightness(brightness + 1);

      if (debug) {
        Serial.print("DIM DOWN decr: ");
        Serial.println(brightness + 1);
      }
    }
    last_millis = millis();
  }

  //light all leds to the designated brightness and sustain the light after releasing for 1 sec
  if (
    ( (millis() - millis_released[2]) < 1000 ||  (millis() - millis_released[3]) < 1000 )
    ||
    ( !buttonReleased(2) ||  !buttonReleased(3) )

  ) {
    if (brightness >= max_brightness) {
      pixels.clear();
      //lightSmileFace();
      Adi();
      pixels.show();
    }
    else if (brightness == 0) {
      pixels.clear();
      //lightSadFace();
      Adi();
      pixels.show();
    }
    else {
      pixels.clear();
      if (first_splash) {
        Adi();
      }
      else {
        lightFullLeds();
      }
      pixels.show();
    }
  }
  else {
    first_splash = 0;
    MSGEQ7_read();

    pixels.clear();
    pixels.show();

    ledLights(spectrumValue, countArr(spectrumValue), modeType);

    pixels.show();
  }

}

void setColors(uint8_t colorType) {
  switch (colorType) {
    case 0:
      colors[0] = pixels.Color(153, 0, 0);
      colors[1] = pixels.Color(153, 0, 0);
      colors[2] = pixels.Color(153, 0, 0);
      colors[3] = pixels.Color(153, 0, 0);
      colors[4] = pixels.Color(153, 0, 0);
      colors[5] = pixels.Color(153, 0, 0);
      colors[6] = pixels.Color(153, 0, 0);
      colors[7] = pixels.Color(153, 0, 0);

      peakColor = pixels.Color(0, 0, 153);
      break;
    case 1:
      colors[0] = pixels.Color(0, 153, 0);
      colors[1] = pixels.Color(0, 153, 0);
      colors[2] = pixels.Color(0, 153, 0);
      colors[3] = pixels.Color(0, 153, 0);
      colors[4] = pixels.Color(0, 153, 0);
      colors[5] = pixels.Color(0, 153, 0);
      colors[6] = pixels.Color(0, 153, 0);
      colors[7] = pixels.Color(0, 153, 0);

      peakColor = pixels.Color(153, 0, 0);
      break;
    case 2:
      colors[0] = pixels.Color(0, 0, 153);
      colors[1] = pixels.Color(0, 0, 153);
      colors[2] = pixels.Color(0, 0, 153);
      colors[3] = pixels.Color(0, 0, 153);
      colors[4] = pixels.Color(0, 0, 153);
      colors[5] = pixels.Color(0, 0, 153);
      colors[6] = pixels.Color(0, 0, 153);
      colors[7] = pixels.Color(0, 0, 153);

      peakColor = pixels.Color(153, 0, 0);
      break;
    case 3:
      colors[0] = pixels.Color(0, 0, 153);
      colors[1] = pixels.Color(0, 0, 153);
      colors[2] = pixels.Color(224, 224, 0);
      colors[3] = pixels.Color(224, 224, 0);
      colors[4] = pixels.Color(224, 224, 0);
      colors[5] = pixels.Color(153, 0, 0);
      colors[6] = pixels.Color(153, 0, 0);
      colors[7] = pixels.Color(153, 0, 0);

      peakColor = pixels.Color(153, 0, 0);
      break;
    default:
      colors[0] = pixels.Color(0, 153, 0);
      colors[1] = pixels.Color(0, 153, 0);
      colors[2] = pixels.Color(0, 153, 0);
      colors[3] = pixels.Color(0, 153, 0);
      colors[4] = pixels.Color(0, 153, 0);
      colors[5] = pixels.Color(0, 153, 0);
      colors[6] = pixels.Color(0, 153, 0);
      colors[7] = pixels.Color(0, 153, 0);

      peakColor = pixels.Color(153, 0, 0);
  };
}

void ledLights(uint16_t *spectrumValue, uint8_t sizeSpectrum, uint8_t type) {
  uint8_t amplitudeBits = 0;
  uint8_t max_amplitudeBits = 0;

  //MODE 0 :: lights all leds till amplitude
  if (type == 0) {
    for (int i = 0 ; i < NUM_COLS; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }
      lightAllBand(i, amplitudeBits, colors);
    }
  }

  //MODE 1 :: lights all leds till amplitude + fall max amplitude
  if (type == 1) {
    for (int i = 0 ; i < NUM_COLS; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }
      lightAllBand(i, amplitudeBits, colors);

      if (amplitudeBits >= amplitudeBitsLast[i]) {
        amplitudeBitsLast[i] = amplitudeBits;
        last_millis_fall[i] = millis();
        last_millis_stay_max[i] = millis();
      }
      else if (amplitudeBitsLast[i] > 0 && (millis() - last_millis_fall[i]) > speedFall && (millis() - last_millis_stay_max[i]) > stayMaxFall) {
        amplitudeBitsLast[i]--;
        last_millis_fall[i] = millis();
      }

      if (amplitudeBitsLast[i] > 0) {
        if (cntColors != 3) {
          lightPixelBandColor(i, amplitudeBitsLast[i], peakColor);
        }
        else {
          lightPixelBand(i, amplitudeBitsLast[i], colors);
        }
      }
    }
  }


  //MODE 2 :: fall max amplitude
  if (type == 2) {
    for (int i = 0 ; i < NUM_COLS; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }

      if (amplitudeBits >= amplitudeBitsLast[i]) {
        amplitudeBitsLast[i] = amplitudeBits;
        last_millis_fall[i] = millis();
      }
      else if (amplitudeBitsLast[i] > 0 && (millis() - last_millis_fall[i]) > speedFall_1) {
        amplitudeBitsLast[i]--;
        last_millis_fall[i] = millis();
      }

      if (amplitudeBitsLast[i] > 0) {
        lightPixelBand(i, amplitudeBitsLast[i], colors);
      }
    }
  }

  //MODE 3 :: center maxim frequency and distribute the rest
  if (type == 3) {
    max_amplitudeBits = floor(maxFreq / 128) + 1;
    for (int i = 0 ; i < NUM_COLS; i++) {
      if (maxFreq < filter) {
        max_amplitudeBits = 0;
      }

      //if is center
      if (i == 3) {
        lightAllBand(i, max_amplitudeBits, colors);
      }
      else if (i < 3) {
        lightAllBand(i, (
                       (max_amplitudeBits - (3 - i)) > 0
                       ?
                       (max_amplitudeBits - (3 - i))
                       :
                       0
                     ), colors);
      }
      else {
        lightAllBand(i, (
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

void lightAllBand(uint8_t col, uint8_t amplitude, uint32_t *colors) {
  uint8_t a = 0;

  for (uint8_t i = 8 * col; i < (8 * col + amplitude); i++) {
    pixels.setPixelColor(i, colors[a++]);
  }
}

void lightPixelBand(uint8_t col, uint8_t amplitude, uint32_t *colors) {
  pixels.setPixelColor(8 * col + amplitude - 1, colors[amplitude - 1]);
}

void lightPixelBandColor(uint8_t col, uint8_t amplitude, uint32_t color) {
  pixels.setPixelColor(8 * col + amplitude - 1, color);
}

void lightFullLeds() {
  for (uint8_t i = 0; i < NUM_COLS; i++) {
    lightAllBand(i, 8, colors);
  }
}

void Adi() {
  pixels.setPixelColor(0, colors[0]);
  pixels.setPixelColor(1, colors[1]);
  pixels.setPixelColor(2, colors[2]);
  pixels.setPixelColor(3, colors[3]);
  pixels.setPixelColor(4, colors[4]);
  pixels.setPixelColor(5, colors[5]);
  pixels.setPixelColor(6, colors[6]);

  pixels.setPixelColor(11, colors[3]);
  pixels.setPixelColor(15, colors[7]);

  pixels.setPixelColor(16, colors[0]);
  pixels.setPixelColor(17, colors[1]);
  pixels.setPixelColor(18, colors[2]);
  pixels.setPixelColor(19, colors[3]);
  pixels.setPixelColor(20, colors[4]);
  pixels.setPixelColor(21, colors[5]);
  pixels.setPixelColor(22, colors[6]);

  pixels.setPixelColor(24, colors[0]);
  pixels.setPixelColor(25, colors[1]);
  pixels.setPixelColor(26, colors[2]);

  pixels.setPixelColor(32, colors[0]);
  pixels.setPixelColor(34, colors[2]);

  pixels.setPixelColor(40, colors[0]);
  pixels.setPixelColor(41, colors[1]);
  pixels.setPixelColor(42, colors[2]);
  pixels.setPixelColor(43, colors[3]);
  pixels.setPixelColor(44, colors[4]);
  pixels.setPixelColor(45, colors[5]);
  pixels.setPixelColor(46, colors[6]);
  pixels.setPixelColor(47, colors[7]);

  pixels.setPixelColor(48, colors[0]);
  pixels.setPixelColor(49, colors[1]);
  pixels.setPixelColor(50, colors[2]);
  pixels.setPixelColor(51, colors[3]);
  pixels.setPixelColor(52, colors[4]);
  pixels.setPixelColor(53, colors[5]);
  pixels.setPixelColor(55, colors[7]);

}

void lightSmileFace() {
  pixels.setPixelColor(2, colors[2]);
  pixels.setPixelColor(5, colors[5]);

  pixels.setPixelColor(9, colors[1]);
  pixels.setPixelColor(12, colors[4]);
  pixels.setPixelColor(13, colors[5]);
  pixels.setPixelColor(14, colors[6]);

  pixels.setPixelColor(16, colors[0]);
  pixels.setPixelColor(21, colors[5]);

  pixels.setPixelColor(24, colors[0]);

  pixels.setPixelColor(32, colors[0]);
  pixels.setPixelColor(37, colors[5]);

  pixels.setPixelColor(41, colors[1]);
  pixels.setPixelColor(44, colors[4]);
  pixels.setPixelColor(45, colors[5]);
  pixels.setPixelColor(46, colors[6]);

  pixels.setPixelColor(50, colors[2]);
  pixels.setPixelColor(53, colors[5]);
}

void lightSadFace() {
  pixels.setPixelColor(0, colors[0]);
  pixels.setPixelColor(5, colors[5]);

  pixels.setPixelColor(9, colors[1]);
  pixels.setPixelColor(12, colors[4]);
  pixels.setPixelColor(13, colors[5]);
  pixels.setPixelColor(14, colors[6]);

  pixels.setPixelColor(18, colors[2]);
  pixels.setPixelColor(21, colors[5]);

  pixels.setPixelColor(26, colors[2]);

  pixels.setPixelColor(34, colors[2]);
  pixels.setPixelColor(37, colors[5]);

  pixels.setPixelColor(41, colors[1]);
  pixels.setPixelColor(44, colors[4]);
  pixels.setPixelColor(45, colors[5]);
  pixels.setPixelColor(46, colors[6]);

  pixels.setPixelColor(48, colors[0]);
  pixels.setPixelColor(53, colors[5]);
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

    if (modeType == 3) { //get maximum value of frequencies
      if (i == 0) {
        maxFreq = spectrumValue[i];
      }
      else if (spectrumValue[i] > maxFreq && i <= 1) {
        maxFreq = spectrumValue[i];
      }
    }

    PORTB |= (1 << strobePin); //set STROBE high
    delayMicroseconds(10); // tss = 72 us (see timing diagram)
  }

  
  if (debug) {
    if(buttonReleased(0) || buttonReleased(1)){
      debug_once = 0;
    }
    
    if(!buttonReleased(0) && !buttonReleased(1) && !debug_once){
      debug_spectrumValues = !debug_spectrumValues;
      if(debug_spectrumValues){
        Serial.println("Show spectrum values....");
      }
      else{
        Serial.println("Hide spectrum values....");
      }
      debug_once = 1;
    }
    
    if (debug_spectrumValues) {
      sum_spectrumValues = array_sum(spectrumValue);
      if (sum_spectrumValues > 0) {
        for (uint8_t i = 0; i < countArr(spectrumValue); i++) {
          Serial.print(spectrumValue[i]);
          Serial.print("\t");
        }
        Serial.println();
      }
    }
  }
}

uint16_t array_sum(uint16_t *spectrumValue) {
  uint16_t sum = 0;

  for (uint8_t i = 0; i < 7; i++) {
    sum += spectrumValue[i];
  }

  return sum;
}

ISR(TIMER1_OVF_vect) {
  //every 5 ms check for debounce
  debouncePress();
}

void debouncePress() {

  volatile static uint8_t pins[] = {BUTTON_MODE, BUTTON_COLORS, BUTTON_DIM_DOWN, BUTTON_DIM_UP};
  volatile uint8_t pinb_arr[] = {PIND, PIND, PIND, PINB};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0, 0};

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

boolean buttonReleased(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (released[pin]) {
      return true;
    }

    return false;
  }
}

/*
boolean checkSwitchOFF() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (!buttonReleased(0)) {
      if ((millis() - millis_pressed[0]) >= 1000) {
        pixels.clear();
        pixels.show();
        return true;
      }
    }

    return false;
  }
}
*/


