
#include <util/atomic.h>
#include <Wire.h>
#include <DS3231.h>
#include <Buttons.h>

DS3231 clock;
RTCDateTime dt;

Buttons button_1(8); //PB0
Buttons button_2(7); //PD7
Buttons button_3(6); //PD6

uint32_t ircomm = 0;
uint32_t last_ircomm = 0;

uint16_t shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
uint16_t snapshot_shiftReg16[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
int8_t crtDigit = 0;
uint8_t speed_scroll_set = 200;
uint32_t set_clock_number = 0;

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

uint8_t adi[3] = {
  0b11101110, //A
  0b01111010, //d
  0b01100000 //I
};
volatile boolean scroll_text = 0;
volatile uint8_t scroll_cnt = 0;
uint8_t pos_logo = 0;

uint8_t snapshot_stage = 0;
uint8_t button_0_cycle = 0;
int32_t freeze_number = 0;
int32_t freeze_hour = 0;
int32_t freeze_minute = 0;
int32_t freeze_second = 0;
uint32_t last_millis = 0;
uint32_t first_press_millis = 0;
int8_t number_digits = 0;
boolean show_temperature = 0;
boolean show_date = 0;
int8_t display_mode = 0;

char temperature[3];

volatile uint32_t cnt1 = 0;
boolean isOn = 1;
boolean loop_data = 0;
uint8_t loop_data_cnt = 0;
uint8_t loop_data_show = 3;
volatile boolean one_second = false;
uint16_t freq_multiplex;

char* timp;
char* data;
char bufferTemp[6];

boolean ResetVarsOFF = 0;



boolean show_degree = 1;
int8_t last_temperature;
int8_t current_temperature;

uint32_t oled_millis = 0;
uint16_t stop_oled[] = {5000, 5000, 3000, 1000};

const uint8_t fist[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0xd6, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0c, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x50, 0x0b, 0x00, 0x50, 0x15, 0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00,
  0x00, 0x40, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00,
  0x00, 0xc0, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x06, 0x00,
  0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x08, 0x00, 0x10, 0x80, 0x00,
  0x80, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x40, 0x00, 0x00,
  0x00, 0x04, 0x00, 0x08, 0x80, 0x00, 0x20, 0x00, 0x00, 0x00, 0x04, 0x00,
  0x04, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x80, 0x00,
  0x10, 0x00, 0x20, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x02, 0x80, 0x00, 0x08, 0x00, 0x20, 0x00, 0x02, 0x00,
  0x00, 0x40, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x40, 0x00,
  0x80, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02, 0x80, 0x00, 0x10, 0x00, 0x10,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x50, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x01, 0x40, 0x00, 0x80, 0x00, 0x10, 0x00, 0x02, 0x00, 0x01, 0x40, 0x00,
  0x20, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0xa0, 0x00, 0x08,
  0x00, 0x02, 0x00, 0x01, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x02, 0x00,
  0x00, 0x40, 0x00, 0xc0, 0x00, 0x08, 0x00, 0x02, 0x00, 0x01, 0x20, 0x00,
  0x80, 0x00, 0x08, 0x00, 0x02, 0x00, 0x01, 0x20, 0x00, 0x80, 0x00, 0x08,
  0x00, 0x06, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x08, 0x00, 0x04, 0x00,
  0xa1, 0x08, 0x00, 0x00, 0x00, 0x08, 0x00, 0x4c, 0x02, 0x01, 0x0c, 0x00,
  0x00, 0x01, 0x08, 0x00, 0x08, 0x00, 0x05, 0x03, 0x00, 0x00, 0x02, 0x08,
  0x00, 0x14, 0x80, 0xf4, 0x00, 0x00, 0x00, 0x22, 0x88, 0x00, 0x62, 0x70,
  0x00, 0x04, 0x00, 0x00, 0x0c, 0x18, 0x02, 0xe2, 0x17, 0x00, 0x00, 0x00,
  0x00, 0x70, 0x33, 0x80, 0x19, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xc0,
  0x50, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x37, 0x02, 0x00,
  0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x04, 0x00,
  0x00, 0x00, 0x00, 0x00, 0xae, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xa8, 0x17, 0x80, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68,
  0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00
};




void setup() {
  Serial.begin(115200);
  //Wire.setClock(400000L);

  freq_multiplex = 2500;

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT

  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
  dt = clock.getDateTime();
}



void loop() {

  checkSwitchOnOff();
  if (isOn) {

    ResetVarsOFF = 1;

    if (digits_refresh) {
      dt = clock.getDateTime();
    }

    //press DATE button for more than 2 sec to switch off
    if (button_3.isPressedFor(2000) && display_mode == 3) {
      isOn = 0;
      Serial.println("OFF");
      button_3.released = -1;
      button_3.pressed = 0;
    }

    //SETTING CLOCK
    //if the button 1 is pressed enter the setting stage
    //cycle throw setting pairs SS MM HH
    //on the fourth press release the setting stage
    if (button_1.isPressed() && display_mode == 1) {
      if (button_0_cycle == 3) {
        tens_seconds = 0;
        clock.setDateTime(dt.year, dt.month, dt.day, constrain(freeze_hour, 0, 59), constrain(freeze_minute, 0, 59), constrain(freeze_second, 0, 59)); //set CLOCK
      }
      button_0_cycle = ++button_0_cycle % 4;
      snapshot_stage = button_0_cycle;
      button_1.pressed = 0;
    }

    if (button_0_cycle) {
      settingClock();
    }
    //END SETTING CLOCK
    //SHOW Clock
    if (!button_0_cycle && display_mode != 1 && (button_1.isPressed() || ircomm == 0xFD40BF || loop_data_show == 0)) {
      display_mode = 1;
      show_date = 0;
      show_temperature = 0;
      loop_data_show = 3;
      button_1.pressed = 0;
    }
    //END SHOW Clock

    //SHOW Temperature
    if (!button_0_cycle && (button_2.isPressed() || ircomm == 0xFD609F || loop_data_show == 1) ) {
      show_temperature = !show_temperature;
      display_mode = (show_temperature) ? 2 : 1;
      show_date = 0;
      loop_data_show = 3;
      button_2.pressed = 0;
    }
    //END SHOW Temperature

    //SHOW Date
    if (!button_0_cycle && (button_3.isPressed() || ircomm == 0xFD50AF || loop_data_show == 2) ) {
      show_date = !show_date;
      display_mode = (show_date) ? 3 : 1;
      show_temperature = 0;
      loop_data_show = 3;
      button_3.pressed = 0;
    }
    //END SHOW Date

    //1/2 second ON 1/2 second off
    if (toggle_leds_second && !button_0_cycle && display_mode == 1) {
      PORTB ^= (1 << PB3);
      toggle_leds_second = 0;
    }

    if (digits_refresh && display_mode == 2) {
      sprintf(temperature, "%02d", (int8_t) clock.readTemperature());
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (digits_refresh) {
        if (display_mode == 1) {
          timp = clock.dateFormat("Gis", dt);
        }
        if (display_mode == 3) {
          data = clock.dateFormat("dmy", dt);
        }
      }

      digits_refresh = 0;
    }

  }

  if (!isOn) {
    if (ResetVarsOFF) {
      PORTB &= ~(1 << PB3); //turnOff seconds leds
      button_0_cycle = 0;
      pos_logo = 0;
      scroll_text = 0;
      scroll_cnt = 0;
      display_mode = 0;
      resetClock();
      resetNumbers();

      //lcd.clear();
      //lcd.setBacklight(LOW);
      ResetVarsOFF = 0;
    }

    //press DATE button for more than 2 sec to switch on
    if (button_3.isPressedFor(2000)) {
      button_3.pressed = 0;
      button_3.released = -1;
      //lcd.setBacklight(HIGH);
      Serial.println("ON");
      last_temperature = 0;
      show_degree = 1;

      pos_logo = 0;
      scroll_text = 0;
      scroll_cnt = 0;


      show_date = !show_date;

      isOn = 1;
    }
  }

  //every 1 sec
  if (one_second) {
    one_second = false;

    if (loop_data && display_mode != 0) {

      if (loop_data_cnt == 30) {
        loop_data_show = 1;
      }
      if (loop_data_cnt == 31) {
        loop_data_show = 2;
      }
      if (loop_data_cnt == 32) {
        loop_data_show = 0;
        loop_data_cnt = 0;
      }

      loop_data_cnt++;
    }

  }

}

void checkSwitchOnOff() {
  if (ircomm == 0xFD708F) {
    isOn = !isOn;

    pos_logo = 0;
    scroll_text = 0;
    scroll_cnt = 0;
  }
}

void getSnapshot() {
  for (int i = 0; i < countArr(shiftReg16); i++) {
    snapshot_shiftReg16[i] = shiftReg16[i];
  }
  snapshot_shiftReg16[0] = segm7_digits[0];
}

void setToSnapshot() {
  for (int i = 0; i < countArr(snapshot_shiftReg16); i++) {
    shiftReg16[i] = snapshot_shiftReg16[i];
  }
}

void settingClock() {
  if (button_2.isReleased() == 0) { // if UP button is pressed
    if (button_2.first_press) {
      speed_scroll_set = 200;
    }
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if UP on HH
      freeze_hour = ++freeze_hour % 24;
      snapshot_shiftReg16[5] = segm7_digits[freeze_hour % 10];
      if (freeze_hour > 9) {
        snapshot_shiftReg16[6] = segm7_digits[freeze_hour / 10];
      }
      else {
        snapshot_shiftReg16[6] = 0;
      }
      last_millis = millis();

      if (button_2.first_press) {
        first_press_millis = millis();
        button_2.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze_minute = ++freeze_minute % 60;
      snapshot_shiftReg16[3] = segm7_digits[freeze_minute % 10];
      snapshot_shiftReg16[4] = segm7_digits[freeze_minute / 10];
      last_millis = millis();
      if (button_2.first_press) {
        first_press_millis = millis();
        button_2.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if UP on SS
      freeze_second = ++freeze_second % 60;
      snapshot_shiftReg16[1] = segm7_digits[freeze_second % 10];
      snapshot_shiftReg16[2] = segm7_digits[freeze_second / 10];
      last_millis = millis();
      if (button_2.first_press) {
        first_press_millis = millis();
        button_2.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    //pressed[1] = 0;
  }

  if (button_3.isReleased() == 0) { // if DOWN button is pressed
    if (button_3.first_press) {
      speed_scroll_set = 200;
    }
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if DOWN on HH
      freeze_hour = --freeze_hour;
      if (freeze_hour < 0) {
        freeze_hour = 23;
      }
      snapshot_shiftReg16[5] = segm7_digits[freeze_hour % 10];
      if (freeze_hour > 9) {
        snapshot_shiftReg16[6] = segm7_digits[freeze_hour / 10];
      }
      else {
        snapshot_shiftReg16[6] = 0;
      }
      last_millis = millis();
      if (button_3.first_press) {
        first_press_millis = millis();
        button_3.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze_minute = --freeze_minute;
      if (freeze_minute < 0) {
        freeze_minute = 59;
      }
      snapshot_shiftReg16[3] = segm7_digits[freeze_minute % 10];
      snapshot_shiftReg16[4] = segm7_digits[freeze_minute / 10];
      last_millis = millis();
      if (button_3.first_press) {
        first_press_millis = millis();
        button_3.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if DOWN on SS
      freeze_second = --freeze_second;
      if (freeze_second < 0) {
        freeze_second = 59;
      }

      snapshot_shiftReg16[1] = segm7_digits[freeze_second % 10];
      snapshot_shiftReg16[2] = segm7_digits[freeze_second / 10];
      last_millis = millis();
      if (button_3.first_press) {
        first_press_millis = millis();
        button_3.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) { //speed up after 1 sec
        speed_scroll_set = 70;
      }
    }
    //pressed[2] = 0;


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

void setTemperatureDigits(char number[]) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg16[1] = 0b10011100;
    shiftReg16[2] = 0b11000110;
    shiftReg16[3] = segm7_digits[number[1] - 48];
    if (number[0] - 48 != 0) {
      shiftReg16[4] = segm7_digits[number[0] - 48];
    }
  }
}

void setDateDigits(char number[]) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    uint8_t cnt = 1;
    resetNumbers();
    for (int8_t i = 5; i >= 0; i--) {
      shiftReg16[cnt] = segm7_digits[number[i] - 48];
      cnt++;
    }
  }
}

void setAdiLogo(uint8_t pos) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg16[pos] = adi[0];
    shiftReg16[pos - 1] = adi[1];
    shiftReg16[pos - 2] = adi[2];
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
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    PORTC &= ~(1 << STCP);

    for (uint8_t i = 0; i < 16; i++) {
      PORTC &= ~(1 << SHCP);
      PORTC &= ~(1 << DS);
      PORTC |= (1 << SHCP);
    }

    PORTC |= (1 << STCP);
  }

}

void resetNumbers() { //reset all digits to NULL (0)
  for (uint8_t i = 0; i < countDigits; i++) {
    shiftReg16[i] = 0;
  }
}



ISR(TIMER1_OVF_vect) {

  static uint32_t cnt = 0;

  // 1/10 seconds
  if (cnt % (freq_multiplex / 10) == 0) {
    tens_seconds = ++tens_seconds % 10;
    //scroll text every 200ms
    //if (tens_seconds % 2 == 0)  {
    if (display_mode == 0) {
      scroll_text = 1;
      if (scroll_cnt <= 12) {
        scroll_cnt++;
      }
    }
    //}
    digits_refresh = 1;
    cnt = 0;
  }
  cnt++;

  //every 1/2 of second
  if (cnt1 % (freq_multiplex / 2) == 0) {
    toggle_leds_second = 1;
  }

  //every 1 sec
  if (cnt1 % freq_multiplex == 0) {
    cnt1 = 0;
    one_second = true;
  }

  cnt1++;

  //check debounce every 5ms
  if ((cnt1 % (int)(0.005 / (1.0 / (float)freq_multiplex))) == 0) {
    button_1.Debounce();
    button_2.Debounce();
    button_3.Debounce();
  }

  if (isOn) {
    digitsRefresh();
  }
}

void digitsRefresh() {
  static uint32_t cnt_refresh = 0;

  resetClock();
  if (display_mode == 0) {
    if (scroll_cnt <= 12) {
      last_millis = 0;
      cnt_refresh = 0;
      if (scroll_text == 1) {
        scroll_text = 0;
        pos_logo++;
      }
      if (pos_logo >= 9) {
        pos_logo = 1;
      }
    }
    else {
      cnt_refresh++;
      if (cnt_refresh > freq_multiplex * 2) { //after 2 seconds go the clock display
        scroll_cnt = 0;
        display_mode = 1;
        cnt_refresh = 0;
      }
    }

    setAdiLogo(pos_logo);
    if (pos_logo - crtDigit >= 0) {
      writeDigits(shiftReg16[pos_logo - crtDigit], pos_logo - crtDigit);
    }

    crtDigit = (++crtDigit) % countArr(adi);
  }

  if (display_mode == 1) {
    char timp_plus_tenth[strlen(timp) + 2];

    sprintf(timp_plus_tenth, "%s%d", timp, tens_seconds);

    number_digits = strlen(timp_plus_tenth);
    setBinaryDigits(timp_plus_tenth, number_digits);

    ////////DISPLAY Setting clock
    if (button_0_cycle) { // HH
      if (snapshot_stage == 1) { //get a snapshot clock
        getSnapshot();
        set_clock_number = atol(timp_plus_tenth);
        freeze_hour = set_clock_number / 100000;
        freeze_minute = (set_clock_number % 100000) / 1000;
        freeze_second = (set_clock_number % 1000) / 10;
        PORTB &= ~(1 << PB3);
      }

      if (button_0_cycle == 1) {
        snapshot_shiftReg16[5] = snapshot_shiftReg16[5] | 1; // show setting state on DOT
      }
      if (button_0_cycle == 2) {
        snapshot_shiftReg16[5] &= ~(1 << 0); //reset the old dot
        snapshot_shiftReg16[3] = snapshot_shiftReg16[3] | 1; // show setting state on DOT
      }
      if (button_0_cycle == 3) {
        snapshot_shiftReg16[3] &= ~(1 << 0); //reset the old dot
        snapshot_shiftReg16[1] = snapshot_shiftReg16[1] | 1; // show setting state on DOT
      }
      setToSnapshot();
      number_digits = (freeze_hour < 10) ? 6 : 7;
    }

    snapshot_stage = 0;
    //////END DISPLAY Setting clock


    writeDigits(shiftReg16[crtDigit], crtDigit);

    crtDigit = (++crtDigit) % number_digits;
  }

  if (display_mode == 2) {
    PORTB &= ~(1 << PB3);
    setTemperatureDigits(temperature);
    crtDigit++;
    if (crtDigit > 4) {
      crtDigit = 1;
    }
    writeDigits(shiftReg16[crtDigit], crtDigit);
  }

  if (display_mode == 3) {
    PORTB |= (1 << PB3);

    setDateDigits(data);
    crtDigit++;
    if (crtDigit > 6) {
      crtDigit = 1;
    }
    writeDigits(shiftReg16[crtDigit], crtDigit);
  }
}


void delayNew(uint32_t tm) {
  uint32_t last_millis = millis();
  while ((millis() - last_millis) < tm) {}
}


