#include <util/atomic.h>
#include <Wire.h>
#include <LiquidTWI2.h>
#include <DS3231.h>

LiquidTWI2 lcd(0x20);
DS3231 clock;
RTCDateTime dt;

uint8_t pressed[] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t lcd_cycle = 0;
uint8_t lcd_on = 0;
uint8_t lamp_on = 0;
uint8_t fan_on = 0;
volatile boolean refreshLCD = 0;
uint32_t ircomm = 0;
volatile boolean set_time_cnt = 1;

volatile uint16_t currentPotValue = 0;
volatile uint16_t lastPotValue = 0;
volatile uint16_t SPEED_FAN = 20;

#include <IRremote.h>
int RECV_PIN = 5;
IRrecv irrecv(RECV_PIN);
decode_results results;


void setup() {

  //Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver

  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008);

  lcd.begin(16, 2);
  lcd.setBacklight(LOW);

  clock.begin();

  cli();
  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR1A = 78; // 200Hz (5ms period)
  TIMSK1 = (1 << TOIE1);

  //setting ADC
  /*
  ADCSRA |= (1 << ADPS2);// Set ADC prescalar to 16 – 1Mhz sample rate @ 16MHz
  ADCSRA &= ~((1 << ADPS1) | (1 << ADPS0)); 
  ADCSRA |= (1 << ADEN);// Enable ADC
  ADCSRA |= (1 << ADIE);// Enable interrupts

  ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
  ADMUX &= ~((1 << MUX0) | (1 << MUX1) | (1 << MUX2) | (1 << MUX3));// Set ADC0 pin PC0
  ADMUX |= (1 << ADLAR);// 8 bit resolution*/

  sei();// Enable Global Interrupts
  //ADCSRA |= (1 << ADSC);// Start A2D Conversions

  DDRD |= (1 << PD7);
  PORTD &= ~(1 << PD7);

  DDRD |= (1 << PD6);
  PORTD &= ~(1 << PD6);

  DDRB &= ~((1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3)); //set PB0-3 as INPUT
  PORTB |= (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3); //set INPUT PULLUP resistor


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
  //displayLogo();

}


void loop() {

  if (irrecv.decode(&results)) {
    ircomm = results.value;
    //Serial.println(ircomm,HEX);
    irrecv.resume();
  }

  switchLamp();
  switchLCD();
  switchFan();
  if (lcd_on) {
    /*if (set_time_cnt) {
      clock.setDateTime(__DATE__, __TIME__);
      set_time_cnt = 0;
    }*/
    if (buttonPressed(1) || ircomm == 0xFFD02F) { //remote button AUX
      lcd.clear();
      lcd_cycle++;
      if (lcd_cycle == 4) {
        lcd_cycle = 0;
      }
      ircomm = 0;
      refreshLCD = 1;
    }
    if (refreshLCD) {
      displayLcd(lcd_cycle);
      refreshLCD = 0;
    }
  }

}

void switchLamp() {
  if (buttonPressed(2) || ircomm == 0xFFF00F) { //remote button CH
    ++lamp_on %= 2;
    if (lamp_on) {
      PORTD |= (1 << PD7);
    }
    else {
      PORTD &= ~(1 << PD7);
    }

    ircomm = 0;
  }

}

void switchFan() {
  if (buttonPressed(3) || ircomm == 0xFF807F) { //remote button DVD
    ++fan_on %= 2;
    if (fan_on) {
      DDRD |= (1 << PD6);
      analogWrite(PD6, SPEED_FAN);
    }
    else {
      DDRD &= ~(1 << PD6);
      PORTD &= ~(1 << PD6);
    }

    ircomm = 0;
  }
  
  if (fan_on) {
    DDRD |= (1 << PD6);
    analogWrite(PD6, SPEED_FAN);
  }
}

void switchLCD() {
  if (buttonPressed(0) || ircomm == 0xFFE01F) { //remote button 3D
    ++lcd_on %= 2;
    if (lcd_on) {
      pressed[1] = 0; // if was cycle lcd when lcd was turn off don't bother consider it
      lcd.setBacklight(HIGH);
      set_time_cnt = 1;
      displayLogo();
    }
    else {
      lcd.setBacklight(LOW);
      lcd.clear();
    }

    ircomm = 0;
  }
}

void displayLogo() {
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(3, 1);
  lcd.write((uint8_t)0);

  lcd.setCursor(5, 1);
  lcd.print("Made by Adi");

  delay(2000);
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

      lcd.setCursor(6, 1);
      sprintf(temp, "Temp: %d%cC", (int8_t) clock.readTemperature(), (char)223);
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

      lcd.setCursor(6, 1);
      sprintf(temp, "Temp: %d%cC", (int8_t) clock.readTemperature(), (char)223);
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

      lcd.setCursor(6, 1);
      sprintf(temp, "Temp: %d%cC", (int8_t) clock.readTemperature(), (char)223);
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

      lcd.setCursor(6, 1);
      sprintf(temp, "Temp: %d%cC", (int8_t) clock.readTemperature(), (char)223);
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

  //Serial.println(ADCH);
  sum += ADCH;
  // make the average from 8 readings
  if (cnt >= 100) {
    currentPotValue = constrain(map((sum / 100), 0, 255, 20, 255), 20, 255);

    if ((currentPotValue < lastPotValue - 2 || currentPotValue > lastPotValue + 2) && currentPotValue != lastPotValue) {
      lastPotValue =  currentPotValue;
      SPEED_FAN = currentPotValue;
      //Serial.println(SPEED_FAN);
    }
    cnt = 0;
    sum = 0;
  }

  ADCSRA |= (1 << ADSC); //Start A2D Conversions
}

void debouncePress() {
  volatile static uint8_t pins[] = {PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7};
  volatile static uint8_t isPushed[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t lastState[] = {0, 0, 0, 0, 0, 0, 0, 0};
  volatile static uint8_t count[] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t i = 0; i < sizeof(pins); i++) {
    if (!pressed[i]) {
      isPushed[i] = ( ( PINB & (1 << pins[i]) ) == 0);

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





