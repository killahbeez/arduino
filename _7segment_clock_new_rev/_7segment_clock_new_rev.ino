
#include <util/atomic.h>
#include <Wire.h>
#include <DS3231.h>
#include <Buttons.h>

DS3231 clock;
RTCDateTime dt;

Buttons button_Clock(8); //PB0
Buttons button_Temp(7); //PD7
Buttons button_Date(6); //PD6

uint8_t shiftReg[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
uint8_t snapshot[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
int8_t crtDigit = 0;
uint8_t speed_scroll_set = 200;
uint32_t set_clock_number = 0;

#define countDigits sizeof(shiftReg)/sizeof(shiftReg[0])
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

uint8_t loop_on[6] = {
  0b00011100, //L
  0b11111100, //O
  0b11111100, //O
  0b11001110, //P
  0b11111100, //O
  0b11101100 //N
};

uint8_t loop_off[7] = {
  0b00011100, //L
  0b11111100, //O
  0b11111100, //O
  0b11001110, //P
  0b11111100, //O
  0b10001110, //F
  0b10001110 //F
};

volatile boolean scroll_text = 0;
volatile uint8_t scroll_cnt = 0;
uint8_t pos_logo = 0;

uint8_t snapshot_stage = 0;
uint8_t button_0_cycle = 0;

typedef struct {
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
} freeze_t;

freeze_t freeze_clock;

uint32_t last_millis = 0;
uint32_t first_press_millis = 0;
int8_t number_digits = 0;
boolean show_temperature = 0;
boolean show_date = 0;

enum {
  LOGO,
  LOOP_ON,
  LOOP_OFF,
  CLOCK,
  TEMP,
  DATE
} display_mode;

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

#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0 | U8G_I2C_OPT_NO_ACK | U8G_I2C_OPT_FAST);


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


void draw(void) {
  static uint32_t current_millis = 0;
  static uint32_t stop_millis = 0;
  static int16_t pixel = 128;
  static boolean stop_anim = 0;
  static uint8_t calup = 0;

  current_millis = millis();
  if (!stop_anim) {

    // graphic commands to redraw the complete screen should be placed here
    if (current_millis - oled_millis > 5) {
      sprintf(temperature, "%02d", (int8_t) clock.readTemperature());
      sprintf(bufferTemp, "%2s", temperature);

      if (pixel > -128) {
        pixel--;
      }
      else {
        pixel = 128;
        calup = ++calup % 3;
      }

      oled_millis = current_millis;
    }
  }


  switch (calup) {
    case 0:
      u8g.setFont(u8g_font_fub49n);
      u8g.drawStr( pixel + 25, 60, bufferTemp);
      u8g.drawDisc( pixel + 115, 15, 7);
      break;
    case 1:
      u8g.setFont(u8g_font_fub20r);
      u8g.drawStr( pixel, 34, clock.dateFormat("l", dt));
      u8g.drawStr( pixel + 30, 64, clock.dateFormat("d/m", dt));
      break;
    case 2:
      pixel = 0;
      u8g.drawXBMP( pixel + 25, 0, 68, 52, fist);
      u8g.setFont(u8g_font_helvR08r);
      u8g.drawStr( pixel + 70, 63, "Made by Adi");
      break;
  }


  if (pixel == 0) {
    if (!stop_anim) {
      stop_millis = current_millis;
    }
    stop_anim = 1;
    if (current_millis - stop_millis > stop_oled[calup]) {
      stop_anim = 0;

      if (calup == 2) {
        pixel = -128;
      }
    }
  }

}


void setup() {
  Serial.begin(115200);

  //Wire.setClock(400000L);
  freq_multiplex = 2000;

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT

  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
  dt = clock.getDateTime();

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }

}



void loop() {
  if (isOn) {
    /*if (!button_0_cycle) {
      u8g.firstPage();
      do {
        draw();
      } while ( u8g.nextPage() );
      }*/

    ResetVarsOFF = 1;
    if (digits_refresh) {
      dt = clock.getDateTime();
    }

    //press DATE button for more than 2 sec to switch off
    if (button_Date.isPressedFor(2000) && display_mode == DATE) {
      isOn = 0;
      Serial.println("OFF");
      button_Date.released = -1;
      button_Date.pressed = 0;
      u8g.sleepOn();
    }

    //press TEMP button for more than 2 sec to setting loop clock/date/temp on or off
    if (button_Temp.isPressedFor(2000) && display_mode == TEMP) {
      loop_data = !loop_data;

      button_Temp.released = -1;
      button_Temp.pressed = 0;

      last_temperature = 0;
      show_degree = 1;

      button_0_cycle = 0;
      pos_logo = 0;
      scroll_text = 0;
      scroll_cnt = 0;

      display_mode = (loop_data == 1) ? LOOP_ON : LOOP_OFF;
      resetClock();
      resetNumbers();

      show_temperature = !show_temperature;
    }

    //SETTING CLOCK
    //if the button 1 is pressed enter the setting stage
    //cycle throw setting pairs SS MM HH
    //on the fourth press release the setting stage
    if (button_Clock.isPressed() && display_mode == CLOCK) {
      if (button_0_cycle == 3) {
        tens_seconds = 0;
        clock.setDateTime(dt.year, dt.month, dt.day, constrain(freeze_clock.hour, 0, 59), constrain(freeze_clock.minute, 0, 59), constrain(freeze_clock.second, 0, 59)); //set CLOCK
      }
      button_0_cycle = ++button_0_cycle % 4;
      snapshot_stage = button_0_cycle;
      button_Clock.pressed = 0;
    }

    if (button_0_cycle) {
      settingClock(snapshot, freeze_clock);
    }
    //END SETTING CLOCK
    //SHOW Clock
    if (!button_0_cycle && display_mode != CLOCK && (button_Clock.isPressed() || loop_data_show == 0)) {
      display_mode = CLOCK;
      show_date = 0;
      show_temperature = 0;
      loop_data_show = 3;
      button_Clock.pressed = 0;
    }
    //END SHOW Clock

    //SHOW Temperature
    if (!button_0_cycle && (button_Temp.isPressed() || loop_data_show == 1) ) {
      show_temperature = !show_temperature;
      display_mode = (show_temperature) ? TEMP : CLOCK;
      show_date = 0;
      loop_data_show = 3;
      button_Temp.pressed = 0;
    }
    //END SHOW Temperature

    //SHOW Date
    if (!button_0_cycle && (button_Date.isPressed() || loop_data_show == 2) ) {
      show_date = !show_date;
      display_mode = (show_date) ? DATE : CLOCK;
      show_temperature = 0;
      loop_data_show = 3;
      button_Date.pressed = 0;
    }
    //END SHOW Date

    //1/2 second ON 1/2 second off
    if (toggle_leds_second && !button_0_cycle && display_mode == CLOCK) {
      PORTB ^= (1 << PB3);
      toggle_leds_second = 0;
    }

    if (digits_refresh && display_mode == TEMP) {
      sprintf(temperature, "%02d", (int8_t) clock.readTemperature());
    }

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      if (digits_refresh) {
        if (display_mode == CLOCK) {
          timp = clock.dateFormat("Gis", dt);
        }
        if (display_mode == DATE) {
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
      display_mode = LOGO;
      resetClock();
      resetNumbers();

      //lcd.clear();
      //lcd.setBacklight(LOW);
      ResetVarsOFF = 0;
    }

    //press DATE button for more than 2 sec to switch on
    if (button_Date.isPressedFor(2000)) {
      button_Date.pressed = 0;
      button_Date.released = -1;
      //lcd.setBacklight(HIGH);
      Serial.println("ON");
      last_temperature = 0;
      show_degree = 1;

      pos_logo = 0;
      scroll_text = 0;
      scroll_cnt = 0;

      u8g.sleepOff();

      show_date = !show_date;

      isOn = 1;
    }
  }

  //every 1 sec
  if (one_second) {
    one_second = false;

    if (loop_data && display_mode != LOGO) {

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

void getSnapshot(uint8_t snapshot[], uint8_t size = 7) {
  for (int i = 0; i < 7; i++) {
    snapshot[i] = shiftReg[i];
  }
  snapshot[0] = segm7_digits[0];
}

void setToSnapshot(uint8_t snapshot[], uint8_t size = 7) {
  for (int i = 0; i < 7; i++) {
    shiftReg[i] = snapshot[i];
  }
}


void settingClock(uint8_t snapshot[], freeze_t &freeze) {
  if (button_Temp.isReleased() == 0) { // if UP button is pressed
    if (button_Temp.first_press) {
      speed_scroll_set = 200;
    }
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if UP on HH
      freeze.hour = ++freeze.hour % 24;
      snapshot[5] = segm7_digits[freeze.hour % 10];
      if (freeze.hour > 9) {
        snapshot[6] = segm7_digits[freeze.hour / 10];
      }
      else {
        snapshot[6] = 0;
      }
      last_millis = millis();

      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze.minute = ++freeze.minute % 60;
      snapshot[3] = segm7_digits[freeze.minute % 10];
      snapshot[4] = segm7_digits[freeze.minute / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if UP on SS
      freeze.second = ++freeze.second % 60;
      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    //pressed[1] = 0;
  }

  if (button_Date.isReleased() == 0) { // if DOWN button is pressed
    if (button_Date.first_press) {
      speed_scroll_set = 200;
    }
    if (button_0_cycle == 1 && diffMillis > speed_scroll_set) { // if DOWN on HH
      freeze.hour = --freeze.hour;
      if (freeze.hour < 0) {
        freeze.hour = 23;
      }
      snapshot[5] = segm7_digits[freeze.hour % 10];
      if (freeze.hour > 9) {
        snapshot[6] = segm7_digits[freeze.hour / 10];
      }
      else {
        snapshot[6] = 0;
      }
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze.minute = --freeze.minute;
      if (freeze.minute < 0) {
        freeze.minute = 59;
      }
      snapshot[3] = segm7_digits[freeze.minute % 10];
      snapshot[4] = segm7_digits[freeze.minute / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (button_0_cycle == 3 && diffMillis > speed_scroll_set) { // if DOWN on SS
      freeze.second = --freeze.second;
      if (freeze.second < 0) {
        freeze.second = 59;
      }

      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
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
      shiftReg[cnt] = segm7_digits[number[i] - 48];
      cnt++;
    }
  }
}

void setTemperatureDigits(char number[]) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg[1] = 0b10011100;
    shiftReg[2] = 0b11000110;
    shiftReg[3] = segm7_digits[number[1] - 48];
    if (number[0] - 48 != 0) {
      shiftReg[4] = segm7_digits[number[0] - 48];
    }
  }
}

void setDateDigits(char number[]) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    uint8_t cnt = 1;
    resetNumbers();
    for (int8_t i = 5; i >= 0; i--) {
      shiftReg[cnt] = segm7_digits[number[i] - 48];
      cnt++;
    }
  }
}

void setAdiLogo(uint8_t pos) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg[pos] = adi[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = adi[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = adi[2];
  }
}


void setLoopON(uint8_t pos) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg[pos] = loop_on[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = loop_on[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = loop_on[2];
    if ( (pos - 3) >= 0) shiftReg[pos - 3] = loop_on[3];
    if ( (pos - 4) >= 0) shiftReg[pos - 4] = loop_on[4];
    if ( (pos - 5) >= 0) shiftReg[pos - 5] = loop_on[5];
  }
}

void setLoopOFF(uint8_t pos) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    resetNumbers();
    shiftReg[pos] = loop_off[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = loop_off[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = loop_off[2];
    if ( (pos - 3) >= 0) shiftReg[pos - 3] = loop_off[3];
    if ( (pos - 4) >= 0) shiftReg[pos - 4] = loop_off[4];
    if ( (pos - 5) >= 0) shiftReg[pos - 5] = loop_off[5];
    if ( (pos - 6) >= 0) shiftReg[pos - 6] = loop_off[6];
  }
}

void writeDigits(uint8_t digit, uint8_t pos) {
  uint8_t crt_bit = 0;
  static uint16_t num_display;

  if (digit != 0) {
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
    shiftReg[i] = 0;
  }
}



ISR(TIMER1_OVF_vect) {


  static uint32_t cnt = 0;

  digits_refresh = 1;

  // 1/10 seconds
  if (cnt % (freq_multiplex / 10) == 0) {
    tens_seconds = ++tens_seconds % 10;
    //scroll text every 200ms
    //if (tens_seconds % 2 == 0)  {
    if (display_mode == LOGO) {
      scroll_text = 1;
      if (scroll_cnt <= 12) {
        scroll_cnt++;
      }
    }

    if (display_mode == LOOP_ON || display_mode == LOOP_OFF) {
      scroll_text = 1;
      if (scroll_cnt <= 6) {
        scroll_cnt++;
      }
    }

    //}
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
    button_Clock.Debounce();
    button_Temp.Debounce();
    button_Date.Debounce();
  }

  if (isOn) {
    digitsRefresh();
  }
}

void digitsRefresh() {
  static uint32_t cnt_refresh = 0;

  resetClock();
  if (display_mode == LOGO) {
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
        display_mode = CLOCK;
        cnt_refresh = 0;
      }
    }

    setAdiLogo(pos_logo);
    if (pos_logo - crtDigit >= 0) {
      writeDigits(shiftReg[pos_logo - crtDigit], pos_logo - crtDigit);
    }

    crtDigit = (++crtDigit) % countArr(adi);
  }

  if (display_mode == LOOP_ON || display_mode == LOOP_OFF) {
    if (scroll_cnt <= 6) {
      last_millis = 0;
      cnt_refresh = 0;
      if (scroll_text == 1) {
        scroll_text = 0;
        pos_logo++;
      }
    }
    else {
      cnt_refresh++;
      if (cnt_refresh > freq_multiplex * 2) { //after 2 seconds go the clock display
        scroll_cnt = 0;
        display_mode = CLOCK;
        cnt_refresh = 0;
      }
    }

    if (display_mode == LOOP_ON) {
      setLoopON(pos_logo);
    }
    else {
      setLoopOFF(pos_logo);
    }
    if (pos_logo - crtDigit >= 0) {
      writeDigits(shiftReg[pos_logo - crtDigit], pos_logo - crtDigit);
    }

    if (display_mode == LOOP_ON) {
      crtDigit = (++crtDigit) % countArr(loop_on);
    }
    else {
      crtDigit = (++crtDigit) % countArr(loop_off);
    }
  }

  if (display_mode == CLOCK) {
    char timp_plus_tenth[strlen(timp) + 2];

    sprintf(timp_plus_tenth, "%s%d", timp, tens_seconds);

    number_digits = strlen(timp_plus_tenth);
    setBinaryDigits(timp_plus_tenth, number_digits);

    ////////DISPLAY Setting clock
    if (button_0_cycle) { // HH
      if (snapshot_stage == 1) { //get a snapshot clock
        getSnapshot(snapshot);
        set_clock_number = atol(timp_plus_tenth);
        freeze_clock.hour = set_clock_number / 100000;
        freeze_clock.minute = (set_clock_number % 100000) / 1000;
        freeze_clock.second = (set_clock_number % 1000) / 10;
        PORTB &= ~(1 << PB3);
      }

      if (button_0_cycle == 1) {
        snapshot[5] = snapshot[5] | 1; // show setting state on DOT
      }
      if (button_0_cycle == 2) {
        snapshot[5] &= ~(1 << 0); //reset the old dot
        snapshot[3] = snapshot[3] | 1; // show setting state on DOT
      }
      if (button_0_cycle == 3) {
        snapshot[3] &= ~(1 << 0); //reset the old dot
        snapshot[1] = snapshot[1] | 1; // show setting state on DOT
      }
      setToSnapshot(snapshot);
      number_digits = (freeze_clock.hour < 10) ? 6 : 7;
    }

    snapshot_stage = 0;
    //////END DISPLAY Setting clock


    writeDigits(shiftReg[crtDigit], crtDigit);

    crtDigit = (++crtDigit) % number_digits;
  }

  if (display_mode == TEMP) {
    PORTB &= ~(1 << PB3);
    setTemperatureDigits(temperature);
    crtDigit++;
    if (crtDigit > 4) {
      crtDigit = 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
  }

  if (display_mode == DATE) {
    PORTB |= (1 << PB3);

    setDateDigits(data);
    crtDigit++;
    if (crtDigit > 6) {
      crtDigit = 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
  }
}


void delayNew(uint32_t tm) {
  uint32_t last_millis = millis();
  while ((millis() - last_millis) < tm) {}
}


