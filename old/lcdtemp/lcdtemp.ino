
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
volatile boolean refreshLCD = 0;

#include <IRremote.h>
int RECV_PIN = 6;
IRrecv irrecv(RECV_PIN);
decode_results results;


void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  
  // set the LCD type
  lcd.setMCPType(LTI_TYPE_MCP23008);

  lcd.begin(16, 2);
  lcd.setBacklight(LOW);

  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);

  TCCR2A = (1 << WGM20) | (1 << WGM21);
  TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20) | (1 << WGM22); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR2A = 78; // 200Hz (5ms period)
  TIMSK2 = (1 << TOIE2);
  

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR1A = 15625; // 1Hz (1s period)
  TIMSK1 = (1 << TOIE1);

  DDRD |= (1 << PD7);
  PORTD &= ~(1 << PD7);

  DDRB &= ~(1 << PB0); //set PB0 as INPUT
  PORTB |= (1 << PB0); //set INPUT PULLUP resistor

  DDRB &= ~(1 << PB1); //set PB0 as INPUT
  PORTB |= (1 << PB1); //set INPUT PULLUP resistor

  DDRB &= ~(1 << PB2); //set PB0 as INPUT
  PORTB |= (1 << PB2); //set INPUT PULLUP resistor

  //displayLogo();
  
}

void loop() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    irrecv.resume(); // Receive the next value
  }
  delay(100);

  switchLamp();
  switchLCD();
  if (lcd_on) {
    if (buttonPressed(1)) {
      lcd.clear();
      lcd_cycle++;
      if (lcd_cycle == 4) {
        lcd_cycle = 0;
      }
      refreshLCD = 1;
    }
    if (refreshLCD) {
      displayLcd(lcd_cycle);
      refreshLCD = 0;
    }
  }
  
}

void switchLamp() {
  if (buttonPressed(2)) {
    ++lamp_on %= 2;
    if (lamp_on) {
      PORTD |= (1 << PD7);
    }
    else {
      PORTD &= ~(1 << PD7);
    }
  }

}
void switchLCD() {
  if (buttonPressed(0)) {
    ++lcd_on %= 2;
    if (lcd_on) {
      pressed[1] = 0; // if was cycle lcd when lcd was turn off don't bother consider it
      lcd.setBacklight(HIGH);
      displayLogo();
    }
    else {
      lcd.setBacklight(LOW);
      lcd.clear();
    }
  }
}

void displayLogo() {
  lcd.clear();
  lcd.setBacklight(HIGH);
  lcd.setCursor(0, 0);
  lcd.print("Made by");

  lcd.setCursor(13, 1);
  lcd.print("Adi");
  delay(2000);
  lcd.clear();
}

void displayLcd(uint8_t type) {
  clock.forceConversion();

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
      lcd.print("Temp: ");
      lcd.setCursor(11, 1);
      lcd.print(clock.readTemperature());
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
      lcd.print("Temp: ");
      lcd.setCursor(11, 1);
      lcd.print(clock.readTemperature());
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
      lcd.print("Temp: ");
      lcd.setCursor(11, 1);
      lcd.print(clock.readTemperature());
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
      lcd.print("Temp: ");
      lcd.setCursor(11, 1);
      lcd.print(clock.readTemperature());
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
      lcd.print("Temp: ");
      lcd.setCursor(11, 1);
      lcd.print(clock.readTemperature());
      break;

  }
}


ISR(TIMER2_OVF_vect) {
  debouncePress();
}


ISR(TIMER1_OVF_vect) {
  refreshLCD = 1;
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





