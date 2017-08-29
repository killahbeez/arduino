
#include <util/atomic.h>
#include <Time.h>

uint16_t shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
int8_t crtDigit = 0;

#define countDigits sizeof(shiftReg16)/sizeof(shiftReg16[0])
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

uint32_t hhmmss;
uint32_t tmst = 1447596330 + 7200;

void setup() {
  setTime(tmst);

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 25000; //10Hz
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  //PORTC &= ~(1<<DS);
  //PORTC &= ~(1 << STCP);
  //PORTC &= ~(1 << SHCP);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT
  resetClock();

}

void loop() {
  //1/2 second ON 1/2 second off
  if(toggle_leds_second){
    PORTB ^= (1 << PB3);
    toggle_leds_second = 0;
  }
  
  if (digits_refresh) {
    resetClock();
    hhmmss = getTime() * 10 + tens_seconds;
    setBinaryDigits(hhmmss);
    writeDigits(shiftReg16[crtDigit]);
    //Serial.println(shiftReg16[crtDigit],BIN);

    crtDigit = (++crtDigit) % (countDigits);

    if (crtDigit == 0) {
      //Serial.println("_______________");
    }

    digits_refresh = 0;
  }
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

void writeDigits(uint16_t number) {
  uint8_t crt_bit = 0;
  PORTC &= ~(1 << STCP);

  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
    crt_bit = (number >> i) & 1;

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

int8_t old_second = 0;
void setBinaryDigits(uint32_t number) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    uint8_t numDigits = getNumDigits(number);
    uint8_t digits[numDigits];

    for (uint8_t i = 0; i < numDigits; i++) {
      digits[i] = number % 10;
      number /= 10;
    }

    //when the seconds change reset tens of seconds to 0
    if (digits[1] != old_second && digits[0] != 0) {
      old_second = digits[1];
      tens_seconds = 0;
      digits[0] = 0;
      digits_refresh = 1;
    }

    resetNumbers(); //start fresh with all digits

    for (uint8_t i = 0; i < numDigits; i++) {
      shiftReg16[i] = segm7_digits[digits[i]];
      /*if (i == 1) {
        shiftReg16[i] |= 1;
      }*/
      shiftReg16[i] <<= 8;
      shiftReg16[i] = shiftReg16[i] | (128 >> i);
      //shiftReg16[i] = digits[i];
    }
  }

}

void resetNumbers() { //reset all digits to NULL (0)
  for (uint8_t i = 0; i < countDigits; i++) {
    shiftReg16[i] = 0;
  }
}

uint8_t getNumDigits(uint32_t number)
{
  int digits = 0;
  while (number) {
    number /= 10;
    digits++;
  }
  return digits;
}

ISR(TIMER1_OVF_vect) {
  digits_refresh = 1;
  static uint32_t cnt = 0;
  static uint32_t cnt1 = 0;
  //if (cnt % 500 == 0) {
    tens_seconds = ++tens_seconds % 10;
    //cnt = 0;
  //}
 // cnt++;

  //every 1/2 of second
  if (cnt1 % 5 == 0) {
    toggle_leds_second = 1;
  }

  //every 1 second
  if (cnt1 % 10 == 0) {
    setTime(tmst++);
    cnt1 = 0;
  }

  cnt1++;
}

uint32_t getTime() {
  char str[] = "       ";
  char tmp[] = "       ";

  if (hour() < 10) {
    strcpy (str, "0");
    strcat(str, ltoa(hour(), tmp, 10));
  }
  else {
    strcpy(str, ltoa(hour(), tmp, 10));
  }

  if (minute() < 10) {
    strcat (str, "0");
  }
  strcat(str, ltoa(minute(), tmp, 10));

  if (second() < 10) {
    strcat (str, "0");
  }
  strcat(str, ltoa(second(), tmp, 10));
  return atol(str);
}

