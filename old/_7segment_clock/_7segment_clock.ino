#include <util/atomic.h>
#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
RTCDateTime dt;

uint16_t shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
int8_t crtDigit = 0;

#define countDigits sizeof(shiftReg16)/sizeof(shiftReg16[0])
#define countArr(number) sizeof(number)/sizeof(number[0])-2
#define DS PC0
#define SHCP PC1
#define STCP PC2

volatile boolean digits_refresh = 0;
volatile uint8_t tens_seconds = 0;
volatile boolean toggle_leds_second = 0;

uint8_t segm7_digits[10] = {
  0b11111100, //0
  0b01100000, //1
  0b11011010, //2
  0b11110010, //3
  0b01100110, //4
  0b10110110, //5
  0b10111110, //6
  0b11100000, //7
  0b11111110, //8
  0b11110110  //9
};


void setup() {
  Serial.begin(9600);

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 125; //2000Hz
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT

  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
  dt = clock.getDateTime();
}

void loop() {
  dt = clock.getDateTime();
  //1/2 second ON 1/2 second off
  if (toggle_leds_second) {
    PORTB ^= (1 << PB3);
    toggle_leds_second = 0;
  }

  if (digits_refresh) {
    resetClock();
    char* timp = clock.dateFormat("Gis", dt);

    char timp_plus_tenth[strlen(timp) + 2];
    char tenth[1];
    strcpy(timp_plus_tenth, timp);
    strcat(timp_plus_tenth, itoa(tens_seconds, tenth, 10));

    setBinaryDigits(timp_plus_tenth, strlen(timp_plus_tenth));
    writeDigits(shiftReg16[crtDigit], crtDigit);

    crtDigit = (++crtDigit) % strlen(timp_plus_tenth);
    digits_refresh = 0;

  }

}

void setBinaryDigits(char number[], uint8_t cnt_array) {
  uint8_t cnt = 0;
  resetNumbers();
  for (int8_t i = cnt_array - 1; i >= 0; i--) {
    shiftReg16[cnt] = segm7_digits[number[i] - 48];
    cnt++;
  }
}

void writeDigits(uint8_t digit, uint8_t pos) {
  uint8_t crt_bit = 0;
  static uint16_t num_display;

  num_display = digit << 8 | 128 >> pos;
  PORTC &= ~(1 << STCP);

  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
    crt_bit = (num_display >> i) & 1;

    if (crt_bit == 1) {
      PORTC |= (1 << DS);
    }
    else {
      PORTC &= ~(1 << DS);
    }

    PORTC |= (1 << SHCP);
  }
  PORTC |= (1 << STCP);

}

void resetClock() {
  PORTC &= ~(1 << STCP);

  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
    PORTC &= ~(1 << DS);
    PORTC |= (1 << SHCP);
  }
  PORTC |= (1 << STCP);

}

void resetNumbers() { //reset all digits to NULL (0)
  for (uint8_t i = 0; i < countDigits; i++) {
    shiftReg16[i] = 0;
  }
}

ISR(TIMER1_OVF_vect) {
  digits_refresh = 1;
  static uint32_t cnt = 0;
  static uint32_t cnt1 = 0;

  if (cnt % 200 == 0) {
    tens_seconds = ++tens_seconds % 10;
    cnt = 0;
  }
  cnt++;

  //every 1/2 of second
  if (cnt1 % 1000 == 0) {
    toggle_leds_second = 1;
  }

  //every 1 second
  if (cnt1 % 2000 == 0) {
    cnt1 = 0;
  }

  cnt1++;
}


