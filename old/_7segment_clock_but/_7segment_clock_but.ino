#include <util/atomic.h>
#include <Wire.h>
#include <DS3231.h>
DS3231 clock;
RTCDateTime dt;

uint16_t shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
uint16_t snapshot_shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
int8_t crtDigit = 0;
uint8_t speed_scroll_set = 170;

#define countDigits sizeof(shiftReg16)/sizeof(shiftReg16[0])
#define countArr(number) sizeof(number)/sizeof(number[0])
#define diffMillis (millis() - last_millis)
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

uint8_t pressed[] = {0, 0, 0};
uint8_t snapshot_stage = 0;
uint8_t button_0_cycle = 0;
int8_t freeze_number = 0;
uint32_t last_millis = 0;

void setup() {
  Serial.begin(9600);

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 125; //2000Hz
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT

  DDRB &= ~(1 << PB0); // PB0 INPUT button 1
  PORTB |= (1 << PB0); //PB0 INPUT_PULLUP

  DDRD &= ~(1 << PD7); //PD7 INPUT button 2
  PORTD |= (1 << PD7); //PD7 INPUT_PULLUP

  DDRD &= ~(1 << PD6); //PD7 INPUT button 3
  PORTD |= (1 << PD6); //PD7 INPUT_PULLUP

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

  //if the button 1 is pressed enter the setting stage
  //cycle throw setting pairs SS MM HH
  //on the fourth press release the setting stage
  if (buttonPressed(0)) {
    if (button_0_cycle == 1) {
      clock.setDateTime(dt.year, dt.month, dt.day, constrain(freeze_number, 0, 23), dt.minute, dt.second); //set CLOCK for HH change
    }
    if (button_0_cycle == 2) {
      clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, constrain(freeze_number, 0, 59), dt.second); //set CLOCK for MM change
    }
    if (button_0_cycle == 3) {
      clock.setDateTime(dt.year, dt.month, dt.day, dt.hour, dt.minute, constrain(freeze_number, 0, 59)); //set CLOCK for SS change
    }
    button_0_cycle = ++button_0_cycle % 4;
    snapshot_stage = button_0_cycle;
    pressed[0] = 0;
  }

  if (buttonPressed(1)) {
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if UP on HH
      freeze_number = ++freeze_number % 24;
      snapshot_shiftReg16[5] = segm7_digits[freeze_number % 10];
      if (freeze_number > 9) {
        snapshot_shiftReg16[6] = segm7_digits[freeze_number / 10];
      }
      else {
        snapshot_shiftReg16[6] = 0;
      }
      last_millis = millis();
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze_number = ++freeze_number % 60;
      snapshot_shiftReg16[3] = segm7_digits[freeze_number % 10];
      snapshot_shiftReg16[4] = segm7_digits[freeze_number / 10];
      last_millis = millis();
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if UP on SS
      freeze_number = ++freeze_number % 60;
      snapshot_shiftReg16[1] = segm7_digits[freeze_number % 10];
      snapshot_shiftReg16[2] = segm7_digits[freeze_number / 10];
      last_millis = millis();
    }
    //pressed[1] = 0;
  }

  if (buttonPressed(2)) {
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if DOWN on HH
      freeze_number = --freeze_number;
      if (freeze_number < 0) {
        freeze_number = 23;
      }
      snapshot_shiftReg16[5] = segm7_digits[freeze_number % 10];
      if (freeze_number > 9) {
        snapshot_shiftReg16[6] = segm7_digits[freeze_number / 10];
      }
      else {
        snapshot_shiftReg16[6] = 0;
      }
      last_millis = millis();
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze_number = --freeze_number;
      if (freeze_number < 0) {
        freeze_number = 59;
      }
      snapshot_shiftReg16[3] = segm7_digits[freeze_number % 10];
      snapshot_shiftReg16[4] = segm7_digits[freeze_number / 10];
      last_millis = millis();
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if DOWN on SS
      freeze_number = --freeze_number;
      if (freeze_number < 0) {
        freeze_number = 59;
      }

      snapshot_shiftReg16[1] = segm7_digits[freeze_number % 10];
      snapshot_shiftReg16[2] = segm7_digits[freeze_number / 10];
      last_millis = millis();
    }
    //pressed[2] = 0;
  }

  if (digits_refresh) {
    resetClock();
    char* timp = clock.dateFormat("Gis", dt);

    char timp_plus_tenth[strlen(timp) + 2];
    char tenth[1];
    strcpy(timp_plus_tenth, timp);
    strcat(timp_plus_tenth, itoa(tens_seconds, tenth, 10));

    setBinaryDigits(timp_plus_tenth, strlen(timp_plus_tenth));

    ////////Setting clock
    if (button_0_cycle == 1) { // HH
      if (snapshot_stage == 1) {
        getSnapshot();
        freeze_number = (timp_plus_tenth[0] - 48) * 10 + (timp_plus_tenth[1] - 48);
      }
      shiftReg16[5] = snapshot_shiftReg16[5] | 1; // show setting state on DOT
      shiftReg16[6] = snapshot_shiftReg16[6];
    }
    if (button_0_cycle == 2) {// MM
      if (snapshot_stage == 2) {
        getSnapshot();
        freeze_number = (timp_plus_tenth[2] - 48) * 10 + (timp_plus_tenth[3] - 48);
      }
      shiftReg16[3] = snapshot_shiftReg16[3] | 1; // show setting state on DOT
      shiftReg16[4] = snapshot_shiftReg16[4];
    }
    if (button_0_cycle == 3) {// SS
      if (snapshot_stage == 3) {
        getSnapshot();
        freeze_number = (timp_plus_tenth[4] - 48) * 10 + (timp_plus_tenth[5] - 48);
      }
      shiftReg16[1] = snapshot_shiftReg16[1] | 1; // show setting state on DOT
      shiftReg16[2] = snapshot_shiftReg16[2];
    }
    snapshot_stage = 0;
    //////END Setting

    writeDigits(shiftReg16[crtDigit], crtDigit);

    crtDigit = (++crtDigit) % strlen(timp_plus_tenth);
    digits_refresh = 0;

  }


}

void getSnapshot() {
  for (int i = 0; i < countArr(shiftReg16); i++) {
    snapshot_shiftReg16[i] = shiftReg16[i];
  }
}

void setBinaryDigits(char number[], uint8_t cnt_array) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    uint8_t cnt = 0;
    resetNumbers();
    for (int8_t i = cnt_array - 1; i >= 0; i--) {
      shiftReg16[cnt] = segm7_digits[number[i] - 48];
      cnt++;
    }
  }
}

void dec2sevenSegm(uint8_t number) {

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

  //check debounce every 5ms
  if (cnt1 % 10 == 0) {
    debouncePress();
  }

  cnt1++;
}


void debouncePress() {

  volatile static uint8_t pins[] = {PB0, PD7, PD6};
  volatile uint8_t pinb_arr[] = {PINB, PIND, PIND};
  volatile static uint8_t isPushed[] = {0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0};

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
      }
    }
    else if (!isPushed[i] && lastState[i]) { //if released
      lastState[i] = isPushed[i];
      pressed[i] = 0;
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

void delayNew(uint32_t tm) {
  uint32_t last_millis = millis();
  while ((millis() - last_millis) < tm) {}
}

