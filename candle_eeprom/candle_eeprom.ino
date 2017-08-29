#include <util/atomic.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Buttons.h>
#include <extEEPROM.h>

#define ENC_CTL  DDRC  //encoder port control
#define ENC_WR  PORTC //encoder port write  
#define ENC_RD  PINC  //encoder port read

Buttons button_enc(16);
Buttons button_min(7);
Buttons button_max(8);

char buffer[50];
char *buffer_ptr;


int8_t mode = 0;
uint8_t max_modes = 3;

#define MOD_STANDARD 0
#define MOD_CANDLE 1
#define MOD_REAL_TIME 2

char* modes[] = {"LIGHT", "CANDLE", "REAL TIME"};


uint32_t last_millis = 0;
uint32_t speed_dim_set = 0;
uint32_t first_press_millis = 0;

boolean calibration = 0;
uint8_t dim = 0;

uint8_t calib_min = 0;
uint8_t calib_max = 0;
uint8_t EEP_calib_min = 0;
uint8_t EEP_calib_max = 0;

volatile boolean change_modes = 0;
volatile boolean capture = 0;

volatile boolean one_second = 0;
uint8_t seconds = 0;


volatile boolean print_capture = 0;

boolean serial_debug = 0;
LiquidCrystal_I2C lcd(0x3F, 16, 2);

const uint32_t totalKBytes = 16;
extEEPROM eep(kbits_16, 1, 16);//device size, number of devices, page size

//for calculating the delay after modes are saved on EEPROM
volatile uint32_t chg_mode_millis = 0;

//the delay after the mode change was produced in ms
uint16_t delay_mode_eeprom = 2000;
volatile boolean saved_mode_EEPROM = 1;


//for calculating the delay after intensity is saved on EEPROM
volatile uint32_t chg_intens_millis = 0;

//the delay after the intensity change was produced in ms
uint16_t delay_intens_eeprom = 2000;
volatile boolean saved_intens_EEPROM = 1;


//EEPROM ADDRESS
#define EEP_ADDR_MODE 0
#define EEP_ADDR_INTENS 1
#define EEP_ADDR_CALIB_MIN 2
#define EEP_ADDR_CALIB_MAX 3
#define EEP_ADDR_CAPTURE_MAX 4 //(2 bytes) 4 and 5

#define EEP_MAXIM_ADDR 2047
uint16_t addr_capture_max = 47;

uint16_t adc_cnt = 47;
uint32_t tmp_millis = 0;


volatile boolean make_capture = 0;
volatile uint8_t ocr_capture = 0;
volatile boolean show_capture = 0;
uint16_t capture_addr = adc_cnt;
boolean only_capture_first = 1;

volatile boolean show_flicker_real_time = 0;

void setup() {
  Serial.begin(9600);

  uint8_t eepStatus = eep.begin(twiClock400kHz);      //go fast!
  int EEP_Mode = -1;
  uint8_t EEP_Intens = 0;

  if (eepStatus) {
    if (serial_debug) {
      Serial.println("EEPROM init error");
    }
    while (1);
  }

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR1A = 125; // 2000Hz 
  TIMSK1 = (1 << TOIE1);

  //********** ADC Config **********
  ADMUX |= ( ( 1 << REFS0 ) | ( 1 << ADLAR ) | ( 1 << MUX1 ) | ( 1 << MUX0 ) );  // left adjust and select ADC3
  ADCSRA |= ( ( 1 << ADEN ) | ( 1 << ADPS2 ) | ( 1 << ADPS1 ) | ( 1 << ADPS0 ) ); // ADC enable and clock divide 16MHz by 128 for 125khz clock ADC frequency

  //enable encoder pins interrupt sources (A0 A1)
  PCMSK1 |= (( 1 << PCINT8 ) | ( 1 << PCINT9 ));
  PCICR |= ( 1 << PCIE1 );

  lcd.init();
  lcd.clear();
  lcd.setBacklight(HIGH);

  logo();

  EEP_Mode = eep.read(EEP_ADDR_MODE);

  if (EEP_Mode >= 0 && EEP_Mode <= max_modes) {
    mode = EEP_Mode;
  }
  else {
    mode = MOD_STANDARD;
  }

  EEP_Intens = eep.read(EEP_ADDR_INTENS);

  if (EEP_Intens >= 0 && EEP_Mode <= 255) {
    dim = EEP_Intens;
  }
  else {
    dim = 0;
  }

  EEP_calib_min = eep.read(EEP_ADDR_CALIB_MIN);
  EEP_calib_max = eep.read(EEP_ADDR_CALIB_MAX);

  buffer_ptr = &buffer[0];

  switch (mode) {
    case MOD_STANDARD:
      sprintf(buffer, "Light", mode);
      lcd_print(0, 0, buffer);

      if (dim > 0 && dim < 255) {
        lcd_print(1, 0, "Intensity:");
      }

      show_intensity(dim, buffer_ptr);
      break;

    case MOD_CANDLE:
      sprintf(buffer, "Candle", mode);
      lcd_print(0, 0, buffer);
      break;

    case MOD_REAL_TIME:
      sprintf(buffer, "Real time", mode);
      lcd_print(0, 0, buffer);
      break;
  }

  DDRB |= (1 << PB3);

  TCCR2A |= (1 << COM2A1);
  TCCR2A &= ~(1 << COM2A0);
  TCCR2B &= 0b11111000;
  TCCR2B |= (1 << CS20); // Phase Correct PWM ,  No prescaller 8 => freq = 16000000/510

  addr_capture_max = get_capture_max();
  //Serial.println(TCCR2A,BIN);
  //Serial.println(TCCR2B,BIN);
}

void loop() {

  //write mode change to EEPROM
  if (!saved_mode_EEPROM && (millis() - chg_mode_millis) >= delay_mode_eeprom) {
    saved_mode_EEPROM = 1;

    eep.write(EEP_ADDR_MODE, mode);

    if (serial_debug) {
      sprintf(buffer, "\tSaved mode to EEPROM: %s", modes[eep.read(EEP_ADDR_MODE)]);
      Serial.println(buffer);
    }
  }

  //write intensity change to EEPROM
  if (!saved_intens_EEPROM && (millis() - chg_intens_millis) >= delay_intens_eeprom) {
    saved_intens_EEPROM = 1;

    eep.write(EEP_ADDR_INTENS, dim);

    if (serial_debug) {
      sprintf(buffer, "\tSaved intensity to EEPROM: %d", eep.read(EEP_ADDR_INTENS));
      Serial.println(buffer);
    }
  }

  ////////////////////////////////
  ///////// MODE STANDARD ////////
  ////////////////////////////////
  if (mode == MOD_STANDARD) {

    //change modes
    if (change_modes) {
      calibration = 0;
      capture = 0;

      lcd.clear();
      sprintf(buffer, "Light", mode);
      lcd_print(0, 0, buffer);

      if (serial_debug) {
        Serial.println("Mode: LIGHT");
      }

      if (dim > 0 && dim < 255) {
        lcd_print(1, 0, "Intensity:");
      }

      show_intensity(dim, buffer_ptr);

      if (serial_debug) {
        lcd_print(0, 9, "UART ON");
      }
      else {
        lcd_clear(0, 8, 8);
      }

      change_modes = 0;

      TCCR2A |= (1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);

    }

    if (button_enc.isPressedFor(1000)) {
      serial_debug = !serial_debug;
      if (serial_debug) {
        lcd_clear(0, 8, 8);
        lcd_print(0, 9, "UART ON");
        Serial.println("Start debugging...");
      }
      else {
        lcd_clear(0, 8, 8);
        lcd_print(0, 8, "UART OFF");
        Serial.println("Stop debugging");
      }
      button_enc.released = -1;
    }

    if (button_min.isReleased() == 0) {
      if (button_min.first_press) {
        speed_dim_set = 300;
        first_press_millis = millis();
      }

      if ((millis() - last_millis) > speed_dim_set) {
        last_millis = millis();

        if ( (millis() - first_press_millis) > 2000) {
          speed_dim_set = 30;
        }

        if (dim > 0) {
          dim--;
        }

        if (button_min.first_press) {
          button_min.first_press = 0;

          if (dim > 0 && dim < 255) {
            lcd_print(1, 0, "Intensity:");
          }
        }

        show_intensity(dim, buffer_ptr);
      }

      chg_intens_millis = millis();
      saved_intens_EEPROM = 0;
    }

    if (button_max.isReleased() == 0) {
      if (button_max.first_press) {
        speed_dim_set = 300;
        first_press_millis = millis();
      }

      if ((millis() - last_millis) > speed_dim_set) {
        last_millis = millis();

        if ( (millis() - first_press_millis) > 2000) {
          speed_dim_set = 30;
        }

        if (dim < 255) {
          dim++;
        }

        if (button_max.first_press) {
          button_max.first_press = 0;

          if (dim > 0 && dim < 255) {
            lcd_print(1, 0, "Intensity:");
          }
        }

        show_intensity(dim, buffer_ptr);
      }

      chg_intens_millis = millis();
      saved_intens_EEPROM = 0;
    }

    OCR2A = dim;
  }
  ///////// END MODE STANDARD //////////


  ////////////////////////////////
  ///////// MODE CANDLE //////////
  ////////////////////////////////
  if (mode == MOD_CANDLE) {

    //change modes
    if (change_modes) {
      lcd.clear();
      sprintf(buffer, "Candle", mode);
      lcd_print(0, 0, buffer);

      change_modes = 0;

      TCCR2A |= (1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);

      if (serial_debug) {
        Serial.println("Mode: CANDLE");
      }
    }

    // *************** CALIBRATION ******************** //
    if (!capture) {
      if (button_min.isPressedFor(2000) && button_max.isPressedFor(2000)) {
        calibration = !calibration;
        if (calibration) {
          TCCR2A &= ~ ( (1 << COM2A1) | (1 << COM2A0) );

          calib_min = EEP_calib_min;
          calib_max = EEP_calib_max;

          lcd_clear(0, 9, 7);
          lcd_print(0, 9, "Chg Cal");
          lcd_clear(1, 0, 16);

          sprintf(buffer, "Min %d", EEP_calib_min);
          lcd_print(1, 0, buffer);

          sprintf(buffer, "Max %d", EEP_calib_max);
          lcd_print(1, 9, buffer);

          if (serial_debug) {
            sprintf(buffer, "Change calibration, Min:%d Max:%d", EEP_calib_min, EEP_calib_max);
            Serial.println(buffer);
          }
        }
        else {
          TCCR2A |= (1 << COM2A1);
          TCCR2A &= ~(1 << COM2A0);

          lcd_clear(0, 9, 7);
          lcd_clear(1, 0, 16);
          lcd_print(1, 6, "Cancel Cal");

          if (serial_debug) {
            Serial.println("Cancel calibration");
          }
        }
        button_min.released = -1;
        button_max.released = -1;
      }

      if (calibration) {
        if (button_min.isClicked()) {
          calib_min = get_adc();

          lcd_clear(1, 0, 7);
          sprintf(buffer, "Min*%d", calib_min);
          lcd_print(1, 0, buffer);

          if (serial_debug) {
            sprintf(buffer, "Calibrate Temp Min: %d", calib_min );
            Serial.println(buffer);
          }
        }

        if (button_max.isClicked()) {
          calib_max = get_adc();

          lcd_clear(1, 9, 7);
          sprintf(buffer, "Max*%d", calib_max);
          lcd_print(1, 9, buffer);

          if (serial_debug) {
            sprintf(buffer, "Calibrate Temp Max: %d", calib_max );
            Serial.println(buffer);
          }
        }

        if (button_enc.isClicked()) {
          TCCR2A |= (1 << COM2A1);
          TCCR2A &= ~(1 << COM2A0);

          save_calibration();
          calibration = 0;
        }
      }
    }

    // *************** CAPTURE ******************** //
    if (!calibration) {
      if (button_enc.isPressedFor(1000)) {
        TCCR2A &= ~ ( (1 << COM2A1) | (1 << COM2A0) );

        capture = 1;
        lcd_clear(0, 9, 7);
        lcd_clear(1, 0, 16);
        lcd_print(1, 0, "Start capture...");
        seconds = 0;
        button_enc.released = -1;
        adc_cnt = 47;
        tmp_millis = millis();

        if (serial_debug) {
          Serial.println("Start capturing");
        }
      }

      if ( (button_enc.isClicked() || adc_cnt >= EEP_MAXIM_ADDR) && capture) {
        TCCR2A |= (1 << COM2A1);
        TCCR2A &= ~(1 << COM2A0);

        capture = 0;
        finish_capture();
      }

      if (capture && print_capture) {
        lcd_clear(0, 9, 7);
        lcd_print(0, 12, (String) adc_cnt);
        print_capture = 0;

        if (serial_debug) {
          Serial.print(".");
        }
      }

      if (make_capture) {
        make_capture = 0;

        if (eep.write(adc_cnt, ocr_capture) != 0 && serial_debug) {
          Serial.println("Capture write Error");
        }

        /*
        if(serial_debug){
          Serial.print(adc_cnt);
          Serial.print("\t");
          Serial.println(ocr_capture);
        }
        */
      }
    }

    if (show_capture/* && only_capture_first*/) {
      OCR2A = eep.read(capture_addr);
      /*
      if(serial_debug){
        //Serial.print(capture_addr);
        //Serial.print("\t");
        //Serial.println(eep.read(capture_addr));
      }
      */

      if (capture_addr < addr_capture_max) {
        capture_addr++;
      }
      else {
        //only_capture_first = 0;
        capture_addr = 47;
      }

      show_capture = 0;
    }

  }
  ///////// END MODE CANDLE //////////

  ////////////////////////////////
  ///////// MODE REAL TIME ///////
  ////////////////////////////////
  if (mode == MOD_REAL_TIME) {

    //change modes
    if (change_modes) {
      calibration = 0;
      capture = 0;

      lcd.clear();
      sprintf(buffer, "Real time", mode);
      lcd_print(0, 0, buffer);

      change_modes = 0;

      TCCR2A |= (1 << COM2A1);
      TCCR2A &= ~(1 << COM2A0);

      if (serial_debug) {
        Serial.println("Mode: REAL TIME");
      }
    }

    if (show_flicker_real_time) {      
      OCR2A = OCR_after_calibration(get_adc());
      show_flicker_real_time = 0;
    }
  }

  ///////// END MODE REAL TIME //////////

}

void save_calibration() {
  lcd_clear(0, 9, 7);
  lcd_print(0, 10, "EEPROM");
  lcd_clear(1, 0, 16);

  if (eep.write(EEP_ADDR_CALIB_MIN, calib_min) == 0 && eep.write(EEP_ADDR_CALIB_MAX, calib_max) == 0) {
    EEP_calib_min = eep.read(EEP_ADDR_CALIB_MIN);
    EEP_calib_max = eep.read(EEP_ADDR_CALIB_MAX);

    sprintf(buffer, "Min %d", EEP_calib_min);
    lcd_print(1, 0, buffer);

    sprintf(buffer, "Max %d", EEP_calib_max);
    lcd_print(1, 9, buffer);

    if (serial_debug) {
      sprintf(buffer, "\tSave calibration to EEPROM Min:%d Max:%d", EEP_calib_min,  EEP_calib_max);
      Serial.println(buffer);
    }
  }
  else {
    lcd_print(1, 0, "ERROR");

    if (serial_debug) {
      Serial.println("ERROR: Calibration was not saved to EEPROM");
    }
  }

}


void finish_capture() {
  write_capture_max(adc_cnt);

  lcd_clear(0, 9, 7);
  lcd_clear(1, 0, 16);
  lcd_print(1, 3, "Saved capture");

  lcd_clear(0, 9, 7);
  lcd_print(0, 10, "EEPROM");

  if (serial_debug) {
    sprintf(buffer, "Save capture to EEPROM till address %d", get_capture_max());
    Serial.println(buffer);
  }
}

void write_capture_max(uint16_t adc_cnt) {
  static uint8_t high_byte;
  static uint8_t low_byte;

  high_byte = adc_cnt >> 8;
  low_byte = adc_cnt & 0xFF;

  eep.write(EEP_ADDR_CAPTURE_MAX, high_byte);
  eep.write(EEP_ADDR_CAPTURE_MAX + 1, low_byte);

  addr_capture_max = adc_cnt;
}


uint16_t get_capture_max() {
  static uint8_t high_byte;
  static uint8_t low_byte;

  high_byte = eep.read(EEP_ADDR_CAPTURE_MAX);
  low_byte = eep.read(EEP_ADDR_CAPTURE_MAX + 1);

  return (high_byte << 8 | low_byte);
}

void show_intensity(uint8_t dim, char* str) {
  static boolean off = 0;
  static boolean max = 0;

  if (dim == 0 && (!off || change_modes)) {
    lcd_clear(1, 0, 16);
    sprintf(str, "OFF", dim);
    lcd_print(1, 0, str);
    off = 1;
  }
  if (dim == 255 && (!max || change_modes)) {
    lcd_clear(1, 0, 16);
    sprintf(str, "MAX", dim);
    lcd_print(1, 0, str);
    max = 1;
  }
  if (dim > 0 && dim < 255) {
    lcd_clear(1, 11, 3);
    sprintf(str, "%d", dim);
    lcd_print(1, 11, str);
    off = 0;
    max = 0;
  }


  if (serial_debug) {
    if (dim > 0 && dim < 255) {
      Serial.print("Intensity: ");
    }
    Serial.println(str);
  }
}


uint8_t capture_adc() {
  static uint8_t adc = 0;
  static uint8_t ocr = 0;

  adc = get_adc();
  ocr = OCR_after_calibration(adc);
  
  make_capture = 1;
  adc_cnt++;

  return ocr;
}

uint8_t OCR_after_calibration(uint8_t adc){
  static int16_t ocr = 0;
  static float multi_fact = 1;
  multi_fact = (float)255.00 / ((float)EEP_calib_max - (float)EEP_calib_min);
  
  if (adc >= EEP_calib_max) {
    ocr = 255;
  }
  else if (adc <= EEP_calib_min) {
    ocr = 0;
  }
  else {
    ocr = round(((float)adc - (float)EEP_calib_min) * (float)multi_fact);
    if (ocr > 255) ocr = 255;
    if (ocr < 0) ocr = 0;
  }

  return ocr;
}

uint8_t get_adc()
{
  ADCSRA |= ( 1 << ADSC );   // start the ADC Conversion
  while ( ADCSRA & ( 1 << ADSC )); // wait for the conversion to be complete
  return ADCH;
}

void logo() {
  lcd_print(1, 5, "Made by Adi");
  delay(2000);
  lcd.clear();
}

void lcd_clear(uint8_t line, uint8_t from, uint8_t dim) {
  char buf[dim + 1];
  lcd.setCursor(from, line);
  for (uint8_t i = 0; i < dim; i++) {
    buf[i] = ' ';
  }
  buf[dim] = NULL;
  lcd.print(buf);
}

void lcd_print(uint8_t line, uint8_t from, String str) {
  lcd.setCursor(from, line);
  lcd.print(str);
}

ISR(TIMER1_OVF_vect) {
  static uint32_t cnt = 0;
  static uint32_t delay_button = (int)(0.005 / (1.0 / (float)2000));
  static uint32_t delay_get_adc = (int)(0.03 / (1 / (float)2000));
  static uint32_t delay_one_second = (int)(1 / (1.0 / (float)2000));
  static uint32_t delay_print_capture = (int)(0.2 / (1.0 / (float)2000));

  cnt++;

  //check debounce
  if ( (cnt % delay_button) == 0) {
    button_enc.Debounce();
    button_min.Debounce();
    button_max.Debounce();
  }

  //get ADC
  if ((cnt % delay_get_adc) == 0 && mode == MOD_CANDLE) {
    if (capture) {
      ocr_capture = capture_adc();
    }
    else {
      show_capture = 1;
    }
  }

  if ((cnt % delay_get_adc) == 0 && mode == MOD_REAL_TIME) {
    show_flicker_real_time = 1;
  }

  //print capture
  if (capture && (cnt % delay_print_capture == 0) ) {
    print_capture = 1;
  }

  //one second
  if (cnt % delay_one_second == 0) {
    one_second = 1;
  }
}

/* encoder routine. Expects encoder with four state changes between detents */
/* and both pins open on detent */
ISR(PCINT1_vect)
{
  static uint8_t old_AB = 3;  //lookup table index
  static int8_t encval = 0;   //encoder value
  static const int8_t enc_states [] PROGMEM = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; //encoder lookup table

  if (!capture) {
    old_AB <<= 2; //remember previous state
    old_AB |= ( ENC_RD & 0x03 );
    encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )]));

    //on detent
    if ( encval > 3 ) { //four steps forward
      mode = ++mode % max_modes;
      encval = 0;
      change_modes = 1;

      saved_mode_EEPROM = 0;
      chg_mode_millis = millis();
    }
    else if ( encval < -3 ) { //four steps backwards
      mode = --mode % max_modes;
      if (mode < 0) {
        mode = max_modes - 1;
      }
      encval = 0;
      change_modes = 1;

      saved_mode_EEPROM = 0;
      chg_mode_millis = millis();
    }
  }
}
