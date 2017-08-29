#include <EEPROM.h>
#include <Wire.h>
#include <DS3231.h>
#include <Buttons.h>

DS3231 clock;
RTCDateTime DS3231_dt;
RTCDate DS3231_date;
RTCTime DS3231_time;
RTCAlarmTime DS3231_alarm;
uint8_t prev_second = 0;
volatile boolean start_of_second = 0;
boolean beep_sec = false;
boolean set_beep_sec = false;
uint8_t dim_led_sec = 180;

Buttons button_Clock(8); //PB0
Buttons button_Temp(7); //PD7
Buttons button_Date(6); //PD6
Buttons button_SetAlarm(2); //PD2
Buttons button_ArmAlarm(5); //PD5

#define countDigits sizeof(shiftReg)/sizeof(shiftReg[0])
#define countArr(number) sizeof(number)/sizeof(number[0])
#define diffMillis (millis() - last_millis)
#define DS PC0
#define SHCP PC1
#define STCP PC2

uint8_t shiftReg[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display
uint8_t snapshot[] = {0, 0, 0, 0, 0, 0, 0}; //7 digits display

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

typedef struct {
  int8_t hour = 0;
  int8_t minute = 0;
  int8_t second = 0;
  int8_t tens = 0;
} freeze_clock_t;

typedef struct {
  uint8_t day = 0;
  uint8_t month = 0;
  int16_t year = 0;
} freeze_date_t;

freeze_clock_t freeze_clock;
freeze_clock_t freeze_alarm;
freeze_date_t freeze_date;

typedef struct buzzer_s {
  uint8_t stage = 0;
  int16_t cnt = 1;
  boolean tick = 0;
} buzzer_t;

volatile buzzer_t buzzer_vars;
volatile buzzer_t buzzer_sec_vars;

volatile struct {
  boolean refresh = false;
  boolean sec_1 = false;
  boolean sec_1_loop = false;
  boolean ms_500 = false;
  boolean ms_500_leds_second = false;
  boolean ms_100 = false;
  boolean ms_10_dim_led = false;
  boolean alarm = false;
  boolean ms_10 = false;
  boolean ms_buzz_5 = false;
  boolean ms_5 = false;
} isr_ticker;

enum {
  LOGO,
  LOOP_ON,
  LOOP_OFF,
  CLOCK,
  TEMP,
  DATE,
  SET_ALARM
} display_mode;


int8_t crtDigit = 0;
uint16_t freq_multiplex;


struct {
  boolean clock = false;
  boolean date = false;
  boolean alarm = false;
} in_setting;


struct {
  uint8_t clock = 0;
  uint8_t date = 0;
  uint8_t alarm = 0;
} setting_cycle;


uint8_t speed_scroll_set = 200;
uint32_t last_millis = 0;
uint32_t first_press_millis = 0;

boolean loop_data = 0;
uint8_t loop_data_cnt = 0;
uint8_t loop_data_show = 3;

uint32_t cnt_refresh = 0;
boolean scroll_text = 0;
uint8_t scroll_cnt = 0;
uint8_t pos_logo = 0;

char* timp;
char* data;
char temperature[2];
char get_alarm[6];

boolean is_armed = false;
boolean alarm_set = 0;
boolean buzzing = false;

struct {
  uint8_t is_armed = 0;
  uint8_t loop = 1;
} EEP_ADDR;

struct {
  boolean is_armed = 0;
  boolean loop = 0;
} EEP_VARS;

void setup() {
  Serial.begin(115200);

  freq_multiplex = 2000;

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);

  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP) | (1 << PC3);
  DDRB |= (1 << PB3); // led toggle seconds OUTPUT

  DDRD |= (1 << PD3); // buzzer
  TCCR2B = (TCCR2B & 0b11111000) | 0b00000010; // set prescaller clk/8 (3.92 kHz)

  DDRD |= (1 << PD0); // UART RX debugger oscilloscope

  clock.begin();
  Wire.setClock(400000L); // SCL freq 400kHz, TWBR = 12

  initialize();

  display_mode = LOGO;

  Serial.println(".......................ready");
}

void initialize() {
  clock.getDate(DS3231_date);
  data = clock.dateFormat("dmy", DS3231_date);

  clock.getTime(DS3231_time);
  timp = clock.dateFormat("Gist", DS3231_time);


  if (EEPROM.read(EEP_ADDR.is_armed) != 255) {
    clock.clearAlarm1();
    EEP_VARS.is_armed = EEPROM.read(EEP_ADDR.is_armed);

    if (EEP_VARS.is_armed) {
      clock.armAlarm1(true);
      is_armed = clock.isArmed1();
    }

    if (is_armed) {
      PORTC |= (1 << PC3);
      Serial.println("init as ARMED");
    }
    else {
      PORTC &= ~(1 << PC3);
      Serial.println("init as NOT ARMED");
    }
  }

  if (EEPROM.read(EEP_ADDR.loop) != 255) {
    loop_data = EEPROM.read(EEP_ADDR.loop);
  }

}

void getSnapshot(uint8_t snapshot[], uint8_t size = 7) {
  for (int i = 0; i < 7; i++) {
    snapshot[i] = shiftReg[i];
  }
  snapshot[0] = 0;
}

void setSR_Snapshot(uint8_t snapshot[], uint8_t size = 7) {
  for (int i = 0; i < 7; i++) {
    shiftReg[i] = snapshot[i];
  }
}

void loop() {

  if (button_Temp.isReleased() == 0 && button_Date.isReleased() == 0) {
    button_Temp.released = -1;
    button_Date.released = -1;
    set_beep_sec = !set_beep_sec;

    display_mode = CLOCK;
    if (set_beep_sec) {
      Serial.println("Set buzzing on sec");
    }
    else {
      Serial.println("UNSet buzzing on sec");
    }
  }

  ////////////////////START setting CLOCK
  if (!in_setting.clock && !in_setting.date && !in_setting.alarm && button_Clock.isPressedFor(1000)) {
    Serial.println("enter setting clock");
    setting_cycle.clock = 1;

    getSnapshot(snapshot);

    clock.getTime(DS3231_time);

    freeze_clock.hour = DS3231_time.hour;
    freeze_clock.minute = DS3231_time.minute;
    freeze_clock.second = DS3231_time.second;
    freeze_clock.tens = 0;

    TCCR2A = TCCR2A & 0b00111111; //led_sec OFF

    button_Clock.pressed = 0;
    button_Clock.released = -1;
    in_setting.clock = true;
  }

  if (in_setting.clock && button_Clock.isPressed() ) {
    if (setting_cycle.clock == 3) {
      clock.setTime(constrain(freeze_clock.hour, 0, 23), constrain(freeze_clock.minute, 0, 59), constrain(freeze_clock.second, 0, 59));
      DS3231_time.tens = 0;
      isr_ticker.ms_100 = true; // show immediatelly the result not waiting for the ISR
      display_mode = CLOCK;
      in_setting.clock = false;
    }
    setting_cycle.clock = ++setting_cycle.clock % 4;
    button_Clock.pressed = 0;
    button_Clock.released = -1;
  }

  if (in_setting.clock && setting_cycle.clock) {
    settingClock(snapshot, freeze_clock);
  }
  ////////////////////END setting CLOCK


  ////////////////////START setting DATE
  if (!in_setting.clock && !in_setting.date && !in_setting.alarm && button_Date.isPressedFor(1000)) {
    Serial.println("enter setting date");
    setting_cycle.date = 1;

    getSnapshot(snapshot);

    clock.getDate(DS3231_date);

    freeze_date.day = DS3231_date.day;
    freeze_date.month = DS3231_date.month;
    freeze_date.year = DS3231_date.year - 2000;

    TCCR2A = TCCR2A & 0b00111111; //led sec OFF

    button_Date.pressed = 0;
    button_Date.released = -1;
    in_setting.date = true;
  }

  if (in_setting.date && button_Clock.isPressed() ) {
    if (setting_cycle.date == 3) {
      clock.setDate(freeze_date.year + 2000, freeze_date.month, freeze_date.day); // set DATE
      isr_ticker.sec_1 = true; // show immediatelly the result not waiting for the ISR
      display_mode = DATE;
      in_setting.date = false;
    }
    setting_cycle.date = ++setting_cycle.date % 4;
    button_Clock.pressed = 0;
    button_Clock.released = -1;
  }

  if (in_setting.date && setting_cycle.date) {
    settingDate(snapshot, freeze_date);
  }
  ////////////////////END setting DATE

  ////////////////////START setting ALARM
  if (in_setting.alarm && !in_setting.clock && !in_setting.date && !alarm_set) {
    Serial.println("enter setting alarm");
    setting_cycle.alarm = 1;


    DS3231_alarm = clock.getAlarm1();
    sprintf(get_alarm, "%02d%02d%02d", DS3231_alarm.hour, DS3231_alarm.minute, DS3231_alarm.second);
    setSR_Alarm(get_alarm);
    getSnapshot(snapshot);

    freeze_alarm.hour = DS3231_alarm.hour;
    freeze_alarm.minute = DS3231_alarm.minute;
    freeze_alarm.second = DS3231_alarm.second;

    TCCR2A = TCCR2A & 0b00111111; //led sec OFF

    alarm_set = 1;
    display_mode = SET_ALARM;
  }

  if (in_setting.alarm && alarm_set && button_Clock.isPressed() ) {
    if (setting_cycle.alarm == 3) {
      saveAlarm();
      alarm_set = 0;
      display_mode = SET_ALARM;
      in_setting.alarm = false;
    }
    setting_cycle.alarm = ++setting_cycle.alarm % 4;
    button_Clock.pressed = 0;
    button_Clock.released = -1;
  }

  if (in_setting.alarm && setting_cycle.alarm) {
    settingAlarm(snapshot, freeze_alarm);
  }
  ////////////////////END setting ALARM

  //CHANGE TO CLOCK
  if (!in_setting.clock && !in_setting.date && !in_setting.alarm && display_mode != CLOCK && (button_Clock.isPressed() || loop_data_show == 0)) {
    Serial.println("pressed Clock");
    isr_ticker.ms_100 = true; // show immediatelly the result not waiting for the ISR
    loop_data_show = 3;
    display_mode = CLOCK;
    button_Clock.pressed = 0;
  }

  //CHANGE TO DATE
  if (!in_setting.clock && !in_setting.date && !in_setting.alarm && display_mode != DATE && (button_Date.isPressed() || loop_data_show == 1)) {
    Serial.println("pressed Date");
    isr_ticker.sec_1 = true; // show immediatelly the result not waiting for the ISR
    loop_data_show = 3;
    display_mode = DATE;
    button_Date.pressed = 0;
  }

  //CHANGE TO TEMP
  if (!in_setting.clock && !in_setting.date && !in_setting.alarm && display_mode != TEMP && (button_Temp.isPressed() || loop_data_show == 2)) {
    Serial.println("pressed Temp");
    isr_ticker.sec_1 = true; // show immediatelly the result not waiting for the ISR
    loop_data_show = 3;
    display_mode = TEMP;
    button_Temp.pressed = 0;
  }

  //CHANGE LOOP ON / OFF
  if (display_mode == TEMP && button_Temp.isPressedFor(1000)) {
    Serial.println("pressed LOOP");
    loop_data = !loop_data;

    cnt_refresh = 0;
    pos_logo = 0;
    scroll_text = 0;
    scroll_cnt = 0;

    display_mode = (loop_data == 1) ? LOOP_ON : LOOP_OFF;
    //resetClock();
    resetNumbers();


    EEPROM.write(EEP_ADDR.loop, loop_data);
    button_Temp.released = -1;
    button_Temp.pressed = 0;
  }

  //CHANGE TO SET ALARM
  if (!in_setting.clock && !in_setting.date && button_SetAlarm.isPressed()) {
    Serial.println("pressed Set Alarm");
    in_setting.alarm = !in_setting.alarm;
    if (!in_setting.alarm) {
      saveAlarm();
      setting_cycle.alarm = 0;
      alarm_set = 0;
    }
    loop_data_show = 3;

    DS3231_alarm = clock.getAlarm1();
    sprintf(get_alarm, "%02d%02d%02d", DS3231_alarm.hour, DS3231_alarm.minute, DS3231_alarm.second);

    display_mode = SET_ALARM;
    button_SetAlarm.pressed = 0;
  }

  //ARM ALARM
  if (!in_setting.clock && !in_setting.date && button_ArmAlarm.isPressed() ) {

    if (!is_armed) {
      clock.clearAlarm1();
      clock.armAlarm1(true);
      is_armed = true;
      
      PORTC |= (1 << PC3);
      EEPROM.write(EEP_ADDR.is_armed, 1);
      Serial.println("armed");
    }
    else {
      clock.clearAlarm1();
      clock.armAlarm1(false);
      is_armed = false;
      
      PORTC &= ~(1 << PC3);
      EEPROM.write(EEP_ADDR.is_armed, 0);
      Serial.println("NOT armed");
    }

    button_ArmAlarm.pressed = 0;
  }
  //END ARM ALARM


  //START ALARM BUZZER
  if (!buzzing && is_armed && isr_ticker.alarm ) {
    if (clock.isAlarm1(false)) {
      TCCR2A = ( TCCR2A & 0b11001111 ) | 0b00100000; //enable PWM for PD3
      buzzing = true;
      Serial.println("BUZZER ALARM......................");
    }
    isr_ticker.alarm = false;
  }

  if (buzzing) {
    buzzer();
  }
  if (buzzing && !is_armed) {
    buzzing = false;
    clock.clearAlarm1();
    buzzer_vars.cnt = 1;
    buzzer_vars.stage = 0;
    TCCR2A = TCCR2A & 0b11001111; //disable PWM for PD3
  }
  //END ALARM BUZZER


  if (isr_ticker.refresh) {
    digitsRefresh();
    isr_ticker.refresh = false;
    //test_digits();
  }

  //START BUZZ every second (countdown bomb style)
  if (!buzzing) {
    if (display_mode == CLOCK) {
      if (set_beep_sec) {
        if (DS3231_time.second != prev_second && !beep_sec) {
          TCCR2A = ( TCCR2A & 0b11001111 ) | 0b00100000; //enable PWM for PD3
          beep_sec = true;
        }
        if (beep_sec) {
          OCR2B = 20;
          if (buzzer_sec_vars.cnt >= 10) { //for 50 ms
            beep_sec = false;
            TCCR2A = TCCR2A & 0b11001111; //disable PWM for PD3
            buzzer_sec_vars.cnt = 0;
          }
          if (isr_ticker.ms_buzz_5) {
            buzzer_sec_vars.cnt++;
            isr_ticker.ms_buzz_5 = false;
          }
        }
      }
      else if (beep_sec) {
        beep_sec = false;
        TCCR2A = TCCR2A & 0b11001111; //disable PWM for PD3
        buzzer_sec_vars.cnt = 0;
      }
    }
    else if (beep_sec) {
      beep_sec = false;
      TCCR2A = TCCR2A & 0b11001111; //disable PWM for PD3
      buzzer_sec_vars.cnt = 0;
    }
  }
  //END BUZZ every second
}


void test_digits() {
  static uint8_t pos = 0;
  pos = ++pos % 8;
  writeDigits(0b11111111, pos);
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
  PORTC &= ~(1 << STCP);

  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
    PORTC &= ~(1 << DS);
    PORTC |= (1 << SHCP);
  }

  PORTC |= (1 << STCP);
}

void resetNumbers() { //reset all digits to NULL (0)
  for (uint8_t i = 0; i < 7; i++) {
    shiftReg[i] = 0;
  }
}


ISR(TIMER1_OVF_vect) {
  static uint32_t cnt = 0;
  static uint32_t cnt_1 = 0;

  //check debounce every 5ms
  if ( cnt % (freq_multiplex / 200) == 0) {
    button_Clock.Debounce();
    button_Temp.Debounce();
    button_Date.Debounce();
    button_SetAlarm.Debounce();
    button_ArmAlarm.Debounce();

    isr_ticker.ms_5 = true;
    isr_ticker.ms_buzz_5 = true;
  }

  // 10 ms ticker
  if (cnt % (freq_multiplex / 100) == 0) {
    isr_ticker.ms_10 = true;
    isr_ticker.ms_10_dim_led = true;

    //change buzzer duty cycle every 10ms
    buzzer_vars.tick = 1;
  }



  // 100 ms ticker
  if (cnt % (freq_multiplex / 10) == 0) {
    DS3231_time.tens = ++DS3231_time.tens % 10;
    if (prev_second != DS3231_time.second && !isr_ticker.ms_100) {
      DS3231_time.tens = 0;
      cnt_1 = 0;
    }
    isr_ticker.ms_100 = true;
  }

  // 500 ms ticker for second leds
  if (cnt_1 % (freq_multiplex / 2) == 0) {
    start_of_second = (cnt_1 == 0 ? true : false);
    isr_ticker.ms_500_leds_second = true;
  }

  // 500 ms ticker
  if (cnt % (freq_multiplex / 2) == 0) {
    isr_ticker.ms_500 = true;
    isr_ticker.alarm = true;
  }

  // 1 second ticker
  if (cnt % freq_multiplex == 0) {
    isr_ticker.sec_1 = true;
    isr_ticker.sec_1_loop = true;
  }

  isr_ticker.refresh = true;

  cnt++;
  cnt_1++;
}

void setSR_Clock(char number[], uint8_t cnt_array = 7) {
  uint8_t cnt = 0;
  resetNumbers();
  for (int8_t i = cnt_array - 1; i >= 0; i--) {
    shiftReg[cnt] = segm7_digits[number[i] - 48];
    cnt++;
  }
}

void setSR_Alarm(char number[]) {
  uint8_t cnt = 1;
  resetNumbers();
  for (int8_t i = 5; i >= 0; i--) {
    shiftReg[cnt] = segm7_digits[number[i] - 48];
    cnt++;
  }
}

void setSR_Date(char number[]) {
  uint8_t cnt = 1;
  resetNumbers();
  for (int8_t i = 5; i >= 0; i--) {
    shiftReg[cnt] = segm7_digits[number[i] - 48];
    cnt++;
  }
}

void setSR_Temp(char number[]) {
  resetNumbers();
  shiftReg[1] = 0b10011100;
  shiftReg[2] = 0b11000110;
  shiftReg[3] = segm7_digits[number[1] - 48];
  if (number[0] - 48 != 0) {
    shiftReg[4] = segm7_digits[number[0] - 48];
  }
}

void setSR_Logo(uint8_t pos) {
  resetNumbers();
  if (pos <= 7) {
    shiftReg[pos] = adi[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = adi[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = adi[2];
  }
}

void setSR_LoopON(uint8_t pos) {
  resetNumbers();

  if (pos <= 7) {
    shiftReg[pos] = loop_on[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = loop_on[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = loop_on[2];
    if ( (pos - 3) >= 0) shiftReg[pos - 3] = loop_on[3];
    if ( (pos - 4) >= 0) shiftReg[pos - 4] = loop_on[4];
    if ( (pos - 5) >= 0) shiftReg[pos - 5] = loop_on[5];
  }
}

void setSR_LoopOFF(uint8_t pos) {
  resetNumbers();
  if (pos <= 7) {
    shiftReg[pos] = loop_off[0];
    if ( (pos - 1) >= 0) shiftReg[pos - 1] = loop_off[1];
    if ( (pos - 2) >= 0) shiftReg[pos - 2] = loop_off[2];
    if ( (pos - 3) >= 0) shiftReg[pos - 3] = loop_off[3];
    if ( (pos - 4) >= 0) shiftReg[pos - 4] = loop_off[4];
    if ( (pos - 5) >= 0) shiftReg[pos - 5] = loop_off[5];
    if ( (pos - 6) >= 0) shiftReg[pos - 6] = loop_off[6];
  }
}



void showLogo() {
  if (isr_ticker.ms_100)  {
    scroll_text = 1;
    if (scroll_cnt <= 12) {
      scroll_cnt++;
    }
    isr_ticker.ms_100 = false;
  }

  if (scroll_cnt <= 12) {
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
      cnt_refresh = 0;
      pos_logo = 0;
      isr_ticker.ms_100 = true;
      display_mode = CLOCK;
    }
  }

  setSR_Logo(pos_logo);
  if (pos_logo - crtDigit >= 0) {
    writeDigits(shiftReg[pos_logo - crtDigit], pos_logo - crtDigit);
  }

  crtDigit = (++crtDigit) % countArr(adi);
}

void showLoop(boolean on = 1) {

  if (isr_ticker.ms_100)  {
    scroll_text = 1;
    if (scroll_cnt <= 6) {
      scroll_cnt++;
    }
    isr_ticker.ms_100 = false;
  }

  if (scroll_cnt <= 6) {
    cnt_refresh = 0;
    if (scroll_text == 1) {
      scroll_text = 0;
      pos_logo++;
    }
  }
  else {
    cnt_refresh++;
    if (cnt_refresh > freq_multiplex) { //after 1 second go the temp display
      scroll_cnt = 0;
      cnt_refresh = 0;
      pos_logo = 0;
      isr_ticker.ms_100 = true;
      display_mode = CLOCK;
    }
  }

  if (on) {
    setSR_LoopON(pos_logo);
    crtDigit = (++crtDigit) % countArr(loop_on);
  }
  else {
    setSR_LoopOFF(pos_logo);
    crtDigit = (++crtDigit) % countArr(loop_off);
  }

  if (pos_logo - crtDigit >= 0) {
    writeDigits(shiftReg[pos_logo - crtDigit], pos_logo - crtDigit);
  }

}

void digitsRefresh() {
  static int8_t number_digits = 7;

  resetClock();
  ///////// SHOW LOGO
  if (display_mode == LOGO) {
    showLogo();
  }
  ///////// SHOW LOOP ON
  if (display_mode == LOOP_ON) {
    showLoop(1);
  }
  ///////// SHOW LOOP OFF
  if (display_mode == LOOP_OFF) {
    showLoop(0);
  }

  ///////// SHOW CLOCK
  if (display_mode == CLOCK) {

    ////////DISPLAY Setting clock
    if (in_setting.clock) {

      if (setting_cycle.clock == 1) {
        snapshot[5] = snapshot[5] | 1; // show setting state on DOT
      }
      if (setting_cycle.clock == 2) {
        snapshot[5] &= ~(1 << 0); //reset the old dot
        snapshot[3] = snapshot[3] | 1; // show setting state on DOT
      }
      if (setting_cycle.clock == 3) {
        snapshot[3] &= ~(1 << 0); //reset the old dot
        snapshot[1] = snapshot[1] | 1; // show setting state on DOT
      }
      setSR_Snapshot(snapshot);
      number_digits = (freeze_clock.hour < 10) ? 6 : 7;
    }
    //////END DISPLAY Setting clock

    if (!in_setting.clock) {
      if (isr_ticker.ms_100) {
        //refresh time from RTC
        prev_second = DS3231_time.second;
        clock.getTime(DS3231_time);

        timp = clock.dateFormat("Gist", DS3231_time);
        number_digits = strlen(timp);
        isr_ticker.ms_100 = false;
      }


      //////START PULSATE THE LED SEC
      if (isr_ticker.ms_500_leds_second) {
        if (start_of_second) { // on second change turn on leds
          OCR2A = dim_led_sec;
        }
        isr_ticker.ms_500_leds_second = false;
      }
      if (isr_ticker.ms_10_dim_led) {
        pulsateLedSec();
        isr_ticker.ms_10_dim_led = false;
      }
      //////END PULSATE THE LED SEC

      setSR_Clock(timp, number_digits);
    }

    if (is_armed && crtDigit == 0) {
      shiftReg[crtDigit] |= 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
    crtDigit = (++crtDigit) % number_digits;
  }


  ////////// SHOW DATE
  if (display_mode == DATE) {

    ////////DISPLAY Setting DATE
    if (in_setting.date) {

      if (setting_cycle.date == 1) {
        snapshot[5] = snapshot[5] | 1; // show setting state on DOT
      }
      if (setting_cycle.date == 2) {
        snapshot[5] &= ~(1 << 0); //reset the old dot
        snapshot[3] = snapshot[3] | 1; // show setting state on DOT
      }
      if (setting_cycle.date == 3) {
        snapshot[3] &= ~(1 << 0); //reset the old dot
        snapshot[1] = snapshot[1] | 1; // show setting state on DOT
      }
      setSR_Snapshot(snapshot);
    }
    //////END DISPLAY Setting DATE

    if (!in_setting.date) {
      if (isr_ticker.sec_1) {
        //refresh date from RTC
        clock.getDate(DS3231_date);

        data = clock.dateFormat("dmy", DS3231_date);
        isr_ticker.sec_1 = false;
      }

      TCCR2A = ( TCCR2A & 0b00111111 ) | 0b10000000; //led sec ON
      OCR2A = dim_led_sec;

      setSR_Date(data);
    }

    crtDigit++;
    if (crtDigit > 6) {
      crtDigit = 0;
    }

    if (is_armed && crtDigit == 0) {
      shiftReg[crtDigit] |= 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
  }

  ////////// SHOW TEMP
  if (display_mode == TEMP) {
    if (isr_ticker.sec_1) {
      sprintf(temperature, "%02d", (int8_t) clock.readTemperature());
      isr_ticker.sec_1 = false;
    }

    TCCR2A = TCCR2A & 0b00111111; //led SEC OFF

    setSR_Temp(temperature);

    crtDigit++;
    if (crtDigit > 4) {
      crtDigit = 0;
    }

    if (is_armed) {
      shiftReg[0] |= 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
  }

  ///////// SHOW ALARM
  if (display_mode == SET_ALARM) {

    ////////DISPLAY Setting alarm
    if (in_setting.alarm) {

      if (setting_cycle.alarm == 1) {
        snapshot[5] = snapshot[5] | 1; // show setting state on DOT
      }
      if (setting_cycle.alarm == 2) {
        snapshot[5] &= ~(1 << 0); //reset the old dot
        snapshot[3] = snapshot[3] | 1; // show setting state on DOT
      }
      if (setting_cycle.alarm == 3) {
        snapshot[3] &= ~(1 << 0); //reset the old dot
        snapshot[1] = snapshot[1] | 1; // show setting state on DOT
      }
      setSR_Snapshot(snapshot);
      number_digits = (freeze_alarm.hour < 10) ? 6 : 7;
    }
    //////END DISPLAY Setting alarm

    if (!in_setting.alarm) {
      TCCR2A = ( TCCR2A & 0b00111111 ) | 0b10000000; //led sec ON
      OCR2A = dim_led_sec;
      setSR_Alarm(get_alarm);
    }

    if (is_armed && crtDigit == 0) {
      shiftReg[crtDigit] |= 1;
    }
    writeDigits(shiftReg[crtDigit], crtDigit);
    crtDigit = (++crtDigit) % number_digits;
  }

  //////// SHOW LOOP DATA
  if (loop_data && display_mode != LOGO && isr_ticker.sec_1_loop) {
    //after 60 sec loop data for 1 sec
    if (loop_data_cnt == 60) {
      loop_data_show = 1;
    }
    if (loop_data_cnt == 61) {
      loop_data_show = 2;
    }
    if (loop_data_cnt == 62) {
      loop_data_show = 0;
      loop_data_cnt = 0;
    }

    loop_data_cnt++;
    isr_ticker.sec_1_loop = false;
  }

}

void pulsateLedSec() {
  TCCR2A = ( TCCR2A & 0b00111111 ) | 0b10000000;
  if (OCR2A <= 3) {
    OCR2A = 0;
  }
  else {
    OCR2A -= 3;
  }
}

void settingClock(uint8_t snapshot[], freeze_clock_t &freeze) {
  if (button_Date.isReleased() == 0) { // if UP button is pressed
    if (button_Date.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.clock == 1 && diffMillis > speed_scroll_set) { // if UP on HH
      freeze.hour = ++freeze.hour % 24;
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
    if (setting_cycle.clock == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze.minute = ++freeze.minute % 60;
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
    if (setting_cycle.clock == 3 && diffMillis > speed_scroll_set) { // if UP on SS
      freeze.second = ++freeze.second % 60;
      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    //pressed[1] = 0;
  }

  if (button_Temp.isReleased() == 0) { // if DOWN button is pressed
    if (button_Temp.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.clock == 1 && diffMillis > speed_scroll_set) { // if DOWN on HH
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
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (setting_cycle.clock == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze.minute = --freeze.minute;
      if (freeze.minute < 0) {
        freeze.minute = 59;
      }
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
    if (setting_cycle.clock == 3 && diffMillis > speed_scroll_set) { // if DOWN on SS
      freeze.second = --freeze.second;
      if (freeze.second < 0) {
        freeze.second = 59;
      }

      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) { //speed up after 1 sec
        speed_scroll_set = 70;
      }
    }
  }
}

void settingAlarm(uint8_t snapshot[], freeze_clock_t &freeze) {
  if (button_Date.isReleased() == 0) { // if UP button is pressed
    if (button_Date.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.alarm == 1 && diffMillis > speed_scroll_set) { // if UP on HH
      freeze.hour = ++freeze.hour % 24;
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
    if (setting_cycle.alarm == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze.minute = ++freeze.minute % 60;
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
    if (setting_cycle.alarm == 3 && diffMillis > speed_scroll_set) { // if UP on SS
      freeze.second = ++freeze.second % 60;
      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    //pressed[1] = 0;
  }

  if (button_Temp.isReleased() == 0) { // if DOWN button is pressed
    if (button_Temp.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.alarm == 1 && diffMillis > speed_scroll_set) { // if DOWN on HH
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
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (setting_cycle.alarm == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze.minute = --freeze.minute;
      if (freeze.minute < 0) {
        freeze.minute = 59;
      }
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
    if (setting_cycle.alarm == 3 && diffMillis > speed_scroll_set) { // if DOWN on SS
      freeze.second = --freeze.second;
      if (freeze.second < 0) {
        freeze.second = 59;
      }

      snapshot[1] = segm7_digits[freeze.second % 10];
      snapshot[2] = segm7_digits[freeze.second / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) { //speed up after 1 sec
        speed_scroll_set = 70;
      }
    }
  }
}

void saveAlarm() {
  // Set Alarm - Every 01h:10m:30s in each day
  // setAlarm1(Date or Day, Hour, Minute, Second, Mode, Armed = true)
  clock.setAlarm1(0, freeze_alarm.hour, freeze_alarm.minute, freeze_alarm.second, DS3231_MATCH_H_M_S, false);
  DS3231_alarm = clock.getAlarm1();
  sprintf(get_alarm, "%02d%02d%02d", DS3231_alarm.hour, DS3231_alarm.minute, DS3231_alarm.second);
  setSR_Alarm(get_alarm);
}

void settingDate(uint8_t snapshot[], freeze_date_t &freeze) {
  if (button_Date.isReleased() == 0) { // if UP button is pressed
    if (button_Date.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.date == 1 && diffMillis > speed_scroll_set) { // if UP on DD
      freeze.day = ++freeze.day;
      if (freeze.day > 31) {
        freeze.day = 1;
      }
      snapshot[5] = segm7_digits[freeze.day % 10];
      if (freeze.day > 9) {
        snapshot[6] = segm7_digits[freeze.day / 10];
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
    if (setting_cycle.date == 2 && diffMillis > speed_scroll_set) { // if UP on MM
      freeze.month = ++freeze.month;
      if (freeze.month > 12) {
        freeze.month = 1;
      }

      snapshot[3] = segm7_digits[freeze.month % 10];
      snapshot[4] = segm7_digits[freeze.month / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (setting_cycle.date == 3 && diffMillis > speed_scroll_set) { // if UP on YY
      freeze.year = ++freeze.year % 100;
      snapshot[1] = segm7_digits[freeze.year % 10];
      snapshot[2] = segm7_digits[freeze.year / 10];
      last_millis = millis();
      if (button_Date.first_press) {
        first_press_millis = millis();
        button_Date.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    //pressed[1] = 0;
  }

  if (button_Temp.isReleased() == 0) { // if DOWN button is pressed
    if (button_Temp.first_press) {
      speed_scroll_set = 200;
    }
    if (setting_cycle.date == 1 && diffMillis > speed_scroll_set) { // if DOWN on DD
      freeze.day = --freeze.day;
      if (freeze.day < 1) {
        freeze.day = 31;
      }
      snapshot[5] = segm7_digits[freeze.day % 10];
      if (freeze.day > 9) {
        snapshot[6] = segm7_digits[freeze.day / 10];
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
    if (setting_cycle.date == 2 && diffMillis > speed_scroll_set) { // if DOWN on MM
      freeze.month = --freeze.month;
      if (freeze.month < 1) {
        freeze.month = 12;
      }
      snapshot[3] = segm7_digits[freeze.month % 10];
      snapshot[4] = segm7_digits[freeze.month / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) {
        speed_scroll_set = 70;
      }
    }
    if (setting_cycle.date == 3 && diffMillis > speed_scroll_set) { // if DOWN on YY
      freeze.year = --freeze.year;
      if (freeze.year < 0) {
        freeze.year = 99;
      }

      snapshot[1] = segm7_digits[freeze.year % 10];
      snapshot[2] = segm7_digits[freeze.year / 10];
      last_millis = millis();
      if (button_Temp.first_press) {
        first_press_millis = millis();
        button_Temp.first_press = 0;
      }

      if ( (millis() - first_press_millis) > 1000) { //speed up after 1 sec
        speed_scroll_set = 70;
      }
    }
  }
}

void buzzer() {
  if (buzzer_vars.tick == 1) {
    if (buzzer_vars.stage == 0) {
      if (buzzer_vars.cnt < 255) {
        buzzer_vars.cnt += 30;
        if (buzzer_vars.cnt > 255) {
          buzzer_vars.cnt = 255;
        }
        OCR2B = buzzer_vars.cnt;
      }
      else {
        buzzer_vars.stage = 1;
        buzzer_vars.cnt = 0;
      }
    }

    if (buzzer_vars.stage == 1) {
      if (buzzer_vars.cnt < 5) {
        buzzer_vars.cnt++;
      }
      else {
        buzzer_vars.stage = 2;
        buzzer_vars.cnt = 255;
      }
    }
    if (buzzer_vars.stage == 2) {
      if (buzzer_vars.cnt > 1 && buzzer_vars.cnt <= 255) {
        buzzer_vars.cnt -= 10;
        if ( buzzer_vars.cnt < 1  ) {
          buzzer_vars.cnt = 1;
        }
        OCR2B = buzzer_vars.cnt;
      }
      else {
        buzzer_vars.stage = 0;
        buzzer_vars.cnt = 1;
      }
    }

    buzzer_vars.tick = 0;
  }
}


void clearDotSettings() {
  shiftReg[5] = shiftReg[5] >> 1 << 1;
  shiftReg[3] = shiftReg[3] >> 1 << 1;
  shiftReg[1] = shiftReg[1] >> 1 << 1;
}

void delayNew(uint32_t tm) {
  uint32_t last_millis = millis();
  while ((millis() - last_millis) < tm) {}
}


