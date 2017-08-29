#include <util/atomic.h>

//Shift register pins
#define ST_CP PD3
#define SH_CP PD4
#define DS PD2
#define OE PB3
#define BUTTON_MODE PB0
#define BUTTON_DIM PC2
#define countArr(number) sizeof(number)/sizeof(number[0])

//MSGEQ7 pins
#define leftChannelPin 0
#define strobePin PB4
#define resetPin PB5

uint8_t pressed[] = {0, 0};
boolean first_press = 1;

uint16_t spectrumLeftValue[7];
uint16_t spectrumRightValue[7];
uint8_t filter = 120;
uint16_t spectrumValue[7];
uint16_t amplitudeBitsLast[7];
uint16_t maxFreq;
uint8_t modeType = 3;

uint16_t speedFall = 60;//the speed (ms) of max amplitude falling
uint16_t stayMaxFall = 200; // how much time (ms) will stay at max amplitude before starting to fall
uint16_t speedFall_1 = 40;


uint32_t last_millis[7];
uint32_t last_millis_stay_max[7];
uint32_t last_millis_1 = 0;

void setup() {
  Serial.begin(9600);
  Serial.end();
  //set STORAGE, SHIFT and DATA IN as OUTPUT for 74HC595N
  DDRD |= (1 << ST_CP) | (1 << SH_CP) | (1 << DS);
  //set OE as OUTPUT  for 74HC595N
  DDRB |= (1 << OE);
  //set LOW for OE
  PORTB &= ~(1 << OE);

  DDRC &= ~(1 << PC0);
  DDRB |= (1 << strobePin) | (1 << resetPin);

  DDRB &= ~(1 << BUTTON_MODE); //BUTTON_MODE INPUT
  PORTB |= (1 << BUTTON_MODE); //BUTTON_MODE INPUT_PULLUP

  DDRC &= ~(1 << BUTTON_DIM); //BUTTON_DIM INPUT
  PORTC |= (1 << BUTTON_DIM); //BUTTON_DIM INPUT_PULLUP

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 1250; //200Hz
  TIMSK1 = (1 << TOIE1);

  //set brightness
  TCCR2A |= (1 << COM2A1) | (1 << COM2A0);
  OCR2A = 1;

  MSGEQ7_reset();
  last_millis_1 = millis();

  setBrightness();
}

void loop() {
  if (buttonPressed(0)) {
    ++modeType %= 4;
    last_millis_1 = millis();
    pressed[0] = 0;
  }

  if (buttonPressed(1)) {
    setBrightness();
    pressed[1] = 0;
  }

  MSGEQ7_read();
  ledLights(spectrumValue, countArr(spectrumValue), modeType);
  
  if(modeType == 1){
    delay(10);
  }
}

void ledLights(uint16_t *spectrumValue, uint8_t sizeSpectrum, uint8_t type) {
  uint8_t amplitudeBits = 0;
  uint8_t max_amplitudeBits = 0;

  PORTD &= ~(1 << ST_CP);

  //MODE 0 :: lights all leds till amplitude
  if (type == 0) {
    for (int i = 0 ; i < sizeSpectrum; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }
      shiftOut(DS, SH_CP, MSBFIRST, round(pow(2, amplitudeBits) - 1));
    }
  }

  //MODE 1 :: combination of previous two
  if (type == 1) {
    for (int i = 0 ; i < sizeSpectrum; i++) {
      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }

      shiftOut(DS, SH_CP, MSBFIRST, (uint8_t) round(pow(2, amplitudeBits) - 1) | (uint8_t) round(pow(2, amplitudeBitsLast[i] - 1)));

      if (amplitudeBits >= amplitudeBitsLast[i]) {
        amplitudeBitsLast[i] = amplitudeBits;
        last_millis[i] = millis();
        last_millis_stay_max[i] = millis();
      }
      else if (amplitudeBitsLast[i] > 0 && (millis() - last_millis[i]) > speedFall && (millis() - last_millis_stay_max[i]) > stayMaxFall) {
        amplitudeBitsLast[i]--;
        last_millis[i] = millis();
      }
    }

  }
  
  //MODE 2 :: light only amplitude led
  if (type == 2) {
    for (int i = 0 ; i < sizeSpectrum; i++) {

      amplitudeBits = floor(spectrumValue[i] / 128) + 1; // 1024 / 8 = 128
      if (spectrumValue[i] < filter) {
        amplitudeBits = 0;
      }

      shiftOut(DS, SH_CP, MSBFIRST, (uint8_t) round(pow(2, amplitudeBitsLast[i] - 1)));

      if (amplitudeBits >= amplitudeBitsLast[i]) {
        amplitudeBitsLast[i] = amplitudeBits;
        last_millis[i] = millis();
      }
      else if (amplitudeBitsLast[i] > 0 && (millis() - last_millis[i]) > speedFall_1) {
        amplitudeBitsLast[i]--;
        last_millis[i] = millis();
      }
    }
  }

  //MODE 3 :: center maxim frequency and distribute the rest
  if (type == 3) { 
    max_amplitudeBits = floor(maxFreq / 128) + 1;

    for (int i = 0 ; i < sizeSpectrum; i++) {
      if (maxFreq < filter) {
        max_amplitudeBits = 0;
      }
      
      //if is center
      if(i == 3){
        shiftOut(DS, SH_CP, MSBFIRST, round(pow(2, max_amplitudeBits) - 1));
      }
      else if(i < 3){
        shiftOut(DS, SH_CP, MSBFIRST, round(pow(2, 
                                                  (
                                                      (max_amplitudeBits - (3-i)) > 0 
                                                          ? 
                                                      (max_amplitudeBits - (3-i))
                                                          : 
                                                          0
                                                   )
                                                 )- 1));
      }
      else{
        shiftOut(DS, SH_CP, MSBFIRST, round(pow(2, 
                                                  (
                                                      (max_amplitudeBits - (i-3)) > 0 
                                                          ? 
                                                      (max_amplitudeBits - (i-3))
                                                          : 
                                                          0
                                                   )
                                                 )- 1));
      }
    }
  }
  
  //latch data
  PORTD |= (1 << ST_CP);
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
  
  for (int8_t i = 6; i >= 0; i--) {
    PORTB &= ~(1 << strobePin); //set STROBE low
    delayMicroseconds(10); // to = 36 us (see timing diagram)
    spectrumValue[i] = analogRead(leftChannelPin);

    if (spectrumValue[i] < filter) {
      spectrumValue[i] = 0;
    }
    
    if(modeType == 3){ //get maximum value of frequencies 
      if(i == 6){
        maxFreq = spectrumValue[i];
      }
      else if(spectrumValue[i] > maxFreq && i >= 5) {
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

  volatile static uint8_t pins[] = {PC2, PB0};
  volatile uint8_t pinb_arr[] = {PINC, PINB};
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
        count[i] = 0;
        first_press = true;
      }
    }
    else if (!isPushed[i] && lastState[i]) { //if released
      lastState[i] = isPushed[i];
      //pressed[i] = 0;
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

void setBrightness() {
  static uint8_t brightness = 0;
  brightness = ++brightness % 5;

  OCR2A = (63 * brightness) + 1;
}

