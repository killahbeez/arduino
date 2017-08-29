#include <util/atomic.h>
#include <Wire.h>
#include <LiquidTWI2.h>
#include <DS3231.h>
//modify wiring.c line 29 to
//#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(prescaler * 256)) to scale accordingly millis()

LiquidTWI2 lcd(0x20);
DS3231 clock;
RTCDateTime dt;

uint8_t pressed[] = {0, 0, 0, 0};
uint8_t lcd_cycle = 0;
uint8_t lcd_on = 0;
uint8_t lamp_on = 0;
uint8_t proc_lamp = 0;
uint8_t fan_on = 0;
uint8_t proc_fan = 0;

volatile boolean refreshLCD = 0;
uint32_t ircomm = 0;
volatile boolean set_time_cnt = 1;

volatile uint16_t currentPotValue = 0;
volatile uint16_t SPEED_FAN = 0;
volatile uint16_t DIM_LIGHT = 0;
volatile boolean updateLF[] = {0, 0};

#include <IRremote.h>
int RECV_PIN = 11;
IRrecv irrecv(RECV_PIN);
decode_results results;

void setup() {

  /*
   * INPUTS (BUTTONS)
   * PD2    LCD ON/OFF
   * PD4    LCD CYCLE
   * PB0    LAMP ON/OFF
   * PD7    FAN ON/OFF
   *
   * ANALOG INPUTS (POT WIPER)
   * PC0 (ADC0)   POT FAN
   * PC1 (ADC1)   POT LAMP
   *
   * PWM OUTPUTS (driving transistor array)
   * PD6 (OC0A)   PWM LAMP
   * PD5 (OC0B)   PWM FAN
   *
   * IR SENSOR
   * PB3 (OC2A)   IR
   */


  //Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver


  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008);

  lcd.begin(16, 2);
  lcd.setBacklight(LOW);

  clock.begin();

  cli();
  TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00); // Fast PWM, Clear OC0A/OC0B on Compare Match, set OC0A/OC0B at BOTTOM
  TCCR0B = (1 << CS00); //clk/1 prescalling freq = 62.5 kHz

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR1A = 78; // 200Hz (5ms period)
  TIMSK1 = (1 << TOIE1);

  //setting ADC

  ADCSRA |= (1 << ADPS2);// Set ADC prescalar to 16 – 1Mhz sample rate @ 16MHz
  ADCSRA &= ~((1 << ADPS1) | (1 << ADPS0));
  ADCSRA |= (1 << ADEN);// Enable ADC
  ADCSRA |= (1 << ADIE);// Enable interrupts

  ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
  ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3));// Set ADC0 pin PC0
  ADMUX |= (1 << ADLAR);// 8 bit resolution

  sei();// Enable Global Interrupts


  ADCSRA |= (1 << ADSC);// Start A2D Conversions

  DDRD &= ~(1 << PD6);
  PORTD &= ~(1 << PD6);

  DDRD &= ~(1 << PD5);
  PORTD &= ~(1 << PD5);

  DDRD &= ~((1 << PD2) | (1 << PD4) | (1 << PD7)); //set PD2 PD4 PD7 as INPUT
  PORTD |= (1 << PD2) | (1 << PD4) | (1 << PD7); //set INPUT PULLUP resistor

  DDRB &= ~(1 << PB0); //set PB0  as INPUT
  PORTB |= (1 << PB0); //set set INPUT PULLUP resistor

  DDRC &= ~(1 << PC0); //set POT FAN as INPUT  (ADC0)
  DDRC &= ~(1 << PC1); //set POT LAMP as INPUT  (ADC1)

  byte fistChar[8] = { // fist
    0b11111,
    0b10101,
    0b10101,
    0b11110,
    0b01001,
    0b01101,
    0b01101,
    0b01101
  };
  lcd.createChar(0, fistChar);

  byte fanChar[8] = {
    0b11111,
    0b01010,
    0b00100,
    0b01110,
    0b01110,
    0b00100,
    0b01010,
    0b11111
  };
  lcd.createChar(1, fanChar);

  byte lamp0Char[8] = {
    0b01110,
    0b10001,
    0b10001,
    0b10001,
    0b01010,
    0b01110,
    0b01010,
    0b01110
  };
  lcd.createChar(2, lamp0Char);



  byte lamp25Char[8] = {
    0b01110,
    0b11001,
    0b11001,
    0b11001,
    0b01010,
    0b01110,
    0b01010,
    0b01110
  };
  lcd.createChar(3, lamp25Char);

  byte lamp50Char[8] = {
    0b01110,
    0b11101,
    0b11101,
    0b11101,
    0b01110,
    0b01110,
    0b01010,
    0b01110
  };
  lcd.createChar(4, lamp50Char);

  byte lamp75Char[8] = {
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b01110,
    0b01010,
    0b01110
  };
  lcd.createChar(5, lamp75Char);

  //displayLogo();
  //clock.setDateTime(__DATE__, __TIME__);
}


void loop() {

  if (irrecv.decode(&results)) {
    ircomm = results.value;
    //Serial.println(ircomm, HEX);
    irrecv.resume();
  }

  switchLamp();
  switchLCD();
  switchFan();
  if (lcd_on) {
    if (buttonPressed(1) || ircomm == 0xFFD02F || ircomm == 0xB38) { //remote button AUX
      lcd.clear();
      lcd_cycle++;
      if (lcd_cycle == 4) {
        lcd_cycle = 0;
      }
      ircomm = 0;
      refreshLCD = 1;
      updateLF[0] = 1;
      updateLF[1] = 1;
      displayFan();
      displayLamp();
    }
    if (refreshLCD) {
      displayLcd(lcd_cycle);
      refreshLCD = 0;
    }
  }

}

void switchLamp() {
  if (buttonPressed(2) || ircomm == 0xFFF00F || ircomm == 0x738) { //remote button CH
    ++lamp_on %= 2;
    if (lamp_on) {
      DDRD |= (1 << PD6);
      OCR0A = DIM_LIGHT;
      //analogWrite(6, DIM_LIGHT);
      updateLF[0] = 1;
    }
    else {
      DDRD &= ~(1 << PD6);
      PORTD &= ~(1 << PD6);
      clearDisplayLamp();
    }

    ircomm = 0;
  }

  if (lamp_on) {

    if ((uint8_t) proc_lamp == 0) {
      DDRD &= ~(1 << PD6);
      PORTD &= ~(1 << PD6);
    }
    else {
      DDRD |= (1 << PD6);
      OCR0A = DIM_LIGHT;
      //analogWrite(6, DIM_LIGHT);
    }

    if (updateLF[0]) {
      displayLamp();
    }
  }
}


void switchFan() {
  if (buttonPressed(3) || ircomm == 0xFF807F || ircomm == 0xF38) { //remote button DVD

    ++fan_on %= 2;
    if (fan_on) {
      DDRD |= (1 << PD5);
      OCR0B = SPEED_FAN;
      //analogWrite(5, SPEED_FAN);
      updateLF[1] = 1;
    }
    else {
      DDRD &= ~(1 << PD5);
      PORTD &= ~(1 << PD5);
      clearDisplayFan();
    }

    ircomm = 0;
  }

  if (fan_on) {

    if ((uint8_t) proc_fan == 0) {
      DDRD &= ~(1 << PD5);
      PORTD &= ~(1 << PD5);
    }
    else {
      DDRD |= (1 << PD5);
      OCR0B = SPEED_FAN;
      //analogWrite(5, SPEED_FAN);
    }

    if (updateLF[1]) {
      displayFan();
    }
  }

}

void displayLamp() {
  char temp[10];
  static uint8_t lampChar = 2;

  //clear previuos display
  if (!lamp_on || updateLF[0]) {
    lcd.setCursor(0, 1);
    lcd.print("    ");
  }

  if (lamp_on && updateLF[0]) {

    proc_lamp = (uint8_t) ((DIM_LIGHT * 100) / 255);
    if (proc_lamp >= 99) proc_lamp = 100;

    lampChar = (uint8_t)((proc_lamp / 25) + 2);
    if (lampChar > 5) lampChar = 5;

    lcd.setCursor(0, 1);
    lcd.write(lampChar);

    sprintf(temp, "%d", (uint8_t) proc_lamp);
    lcd.print(temp);

    updateLF[0] = 0;
  }
}

void clearDisplayLamp() {
  lcd.setCursor(0, 1);
  lcd.print("    ");
}

void displayFan() {
  char temp[10];

  //clear previuos display
  if (!fan_on || updateLF[1]) {
    lcd.setCursor(6, 1);
    lcd.print("    ");
  }

  if (fan_on && updateLF[1]) {
    lcd.setCursor(6, 1);
    lcd.write((uint8_t)1);

    proc_fan = ((SPEED_FAN * 100) / 255);
    if (proc_fan >= 99) proc_fan = 100;

    sprintf(temp, "%d", (uint8_t) proc_fan);
    lcd.print(temp);

    updateLF[1] = 0;
  }
}

void clearDisplayFan() {
  lcd.setCursor(6, 1);
  lcd.print("    ");
}

void switchLCD() {
  if (buttonPressed(0) || ircomm == 0xFFE01F || ircomm == 0x338) { //remote button 3D
    ++lcd_on %= 2;

    if (lcd_on) {
      pressed[1] = 0; // if was cycle lcd when lcd was turn off don't bother consider it
      lcd.setBacklight(HIGH);
      set_time_cnt = 1;
      displayLogo();

      updateLF[0] = 1;
      displayLamp();
      updateLF[1] = 1;
      displayFan();
    }
    else {
      lcd.setBacklight(LOW);
      lcd.clear();
    }

    ircomm = 0;
  }
}

void delayNew(uint32_t tm) {
  uint32_t last_millis = millis();
  while ((millis() - last_millis) < tm) {}
}

void displayLogo() {
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(3, 1);
  lcd.write((uint8_t)0);

  lcd.setCursor(5, 1);
  lcd.print("Made by Adi");

  delayNew(1000);
  lcd.clear();
}

void displayLcd(uint8_t type) {
  clock.forceConversion();
  char temp[10];

  switch (type) {
    //  08-Sep    13:13:11
    //             Temp:28
    case 0:
      dt = clock.getDateTime();
      lcd.setCursor(0, 0);
      lcd.print(clock.dateFormat("d-M", dt));

      lcd.setCursor(8, 0);
      lcd.print(clock.dateFormat("H:i:s", dt));

      lcd.setCursor(12, 1);
      sprintf(temp, "%d%cC", (int8_t) clock.readTemperature(), (char)223);
      lcd.print(temp);
      break;
    //  08-09-2015   13:13
    //             Temp:28
    case 1:
      dt = clock.getDateTime();
      lcd.setCursor(0, 0);
      lcd.print(clock.dateFormat("d-m-Y", dt));

      lcd.setCursor(11, 0);
      lcd.print(clock.dateFormat("H:i", dt));

      lcd.setCursor(12, 1);
      sprintf(temp, "%d%cC", (int8_t) clock.readTemperature(), (char)223);
      lcd.print(temp);
      break;
    //  Duminica   13:13
    //           Temp:28
    case 2:
      dt = clock.getDateTime();
      lcd.setCursor(0, 0);
      lcd.print(clock.dateFormat("l", dt));

      lcd.setCursor(11, 0);
      lcd.print(clock.dateFormat("H:i", dt));

      lcd.setCursor(12, 1);
      sprintf(temp, "%d%cC", (int8_t) clock.readTemperature(), (char)223);
      lcd.print(temp);
      break;
    //  Dum     13:13:50
    //           Temp:28
    case 3:

      dt = clock.getDateTime();
      lcd.setCursor(0, 0);
      lcd.print(clock.dateFormat("D", dt));

      lcd.setCursor(8, 0);
      lcd.print(clock.dateFormat("H:i:s", dt));

      lcd.setCursor(12, 1);
      sprintf(temp, "%d%cC", (int8_t) clock.readTemperature(), (char)223);
      lcd.print(temp);
      break;
    //  08-Sep    13:13:11
    //             Temp:28
    default:
      dt = clock.getDateTime();
      lcd.setCursor(0, 0);
      lcd.print(clock.dateFormat("d-M", dt));

      lcd.setCursor(8, 0);
      lcd.print(clock.dateFormat("H:i:s", dt));

      lcd.setCursor(6, 1);
      sprintf(temp, "Temp: %d%cC", (int8_t) clock.readTemperature(), (char)223);
      lcd.print(temp);
      break;

  }
}


ISR(TIMER1_OVF_vect) {
  static uint8_t cnt = 0;
  debouncePress();

  if (cnt++ == 200) {
    cnt = 0;
    refreshLCD = 1;
  }

}

ISR(ADC_vect) {

  static uint8_t cnt = 0;
  static uint16_t sum = 0;

  cnt++;
  sum += ADCH;
  // make the average from 8 readings

  if (cnt >= 100) {
    currentPotValue = map((sum / 100), 0, 255, 0, 255);
    cnt = 0;
    sum = 0;

    switch (ADMUX & 0x0F) {
      case 0x00:
        setSpeedFan(currentPotValue);
        ADMUX = (ADMUX & 0xE0) | (0x01 & 0x0F);
        break;
      case 0x01:
        setDimLight(currentPotValue);
        ADMUX = (ADMUX & 0xE0) | (0x00 & 0x0F);
        break;
    }


  }
  ADCSRA |= (1 << ADSC); //Start a new ADC

}

void setSpeedFan(uint8_t currentPotValue) {
  static uint16_t lastPotValue = 0;
  if ((currentPotValue < lastPotValue - 2 || currentPotValue > lastPotValue + 2) && currentPotValue != lastPotValue) {
    lastPotValue =  currentPotValue;
    SPEED_FAN = currentPotValue;
    updateLF[1] = 1;
  }
}

void setDimLight(uint8_t currentPotValue) {
  static uint16_t lastPotValue = 0;
  if ((currentPotValue < lastPotValue - 2 || currentPotValue > lastPotValue + 2) && currentPotValue != lastPotValue) {
    lastPotValue =  currentPotValue;
    DIM_LIGHT = currentPotValue;
    updateLF[0] = 1;
  }
}

void debouncePress() {
  volatile static uint8_t pins[] = {PD2, PD4, PB0, PD7};
  volatile uint8_t pinb_arr[] = {PIND, PIND, PINB, PIND};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0, 0};

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    if (!pressed[i]) {
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
        count[i] = 0;
      }
    }
  }
}




boolean buttonPressed(uint8_t pin) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (pressed[pin]) {
      pressed[pin] = 0;
      return true;
    }

    return false;
  }
}





