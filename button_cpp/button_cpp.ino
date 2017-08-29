//////////////////////////////////////////////////////////
/////////////////header file/////////////////////////////
//////////////////////////////////////////////////////////
#define _DDR(P) *portModeRegister(digitalPinToPort(P))
#define _PORT(P) *portOutputRegister(digitalPinToPort(P))
#define _PIN(P) *portInputRegister(digitalPinToPort(P))

#ifndef Buttons_h
#define Buttons_h

class Buttons {
  public:
    boolean pressed = 0;
    int8_t released = -1;

    Buttons(uint8_t pin);

    boolean isPressed();
    int8_t isReleased();
    boolean isPressedFor(uint32_t ms);
    void Debounce();
    uint8_t getPin();

  private:
    uint8_t _pin;
    boolean _isPushed = 0;
    uint8_t _lastState = 0;
    uint8_t _count = 0;
    boolean _first_press = 1;
    uint32_t _millis_pressed;
    uint32_t _millis_released;

};


#endif



///////////////////////////////////////////////////////////////
///////////////class file/////////////////////////////////////
//////////////////////////////////////////////////////////////
Buttons::Buttons(uint8_t pin) {
  _pin = pin;

  //setting IO pin as INPUT with PULL UP resistor set
  _DDR(_pin) &= ~(1 << _pin);
  _PORT(_pin) |= (1 << _pin);
}

boolean Buttons::isPressed() {
  if (pressed) {
    pressed = 0;
    return true;
  }

  return false;
}

boolean Buttons::isPressedFor(uint32_t ms) {
  if (isReleased() == 0 && (millis() - _millis_pressed) > ms) {
    return true;
  }
  return false;
}

int8_t Buttons::isReleased() {
  if (released == 1) {
    return true;
  }

  return released;
}

uint8_t Buttons::getPin() {
  return _pin;
}

void Buttons::Debounce() {
  _isPushed = ( (_PIN(_pin) & (1 << _pin) ) == 0);
  //check if button was pressed
  if (_isPushed && !_lastState) { // if pressed
    _count++;
    // if button has not bounce for 4 checks, the button is debounce and considered pressed
    if (_count >= 4) {
      _lastState = _isPushed;
      pressed = 1;
      released = 0;

      _millis_pressed = millis();
      _count = 0;
      _first_press = true;
    }
  }
  else if (!_isPushed && _lastState) { //if released
    _lastState = _isPushed;
    _millis_released = millis();
    pressed = 0;
    released = 1;
    _count = 0;
  }
  else if (!_isPushed) {
    _count = 0;
  }

}

////////////////////////////////////////////////////////////

volatile uint32_t cnt1 = 0;

Buttons buton_1(PD2);
Buttons buton_2(PD3);
Buttons buton_3(PD4);

void setup() {
  Serial.begin(9600);


  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / 1000;
  TIMSK1 = (1 << TOIE1);
  DDRD &= ~(1 << PD4);
  PORTD |= (1 << PD4);

}

uint8_t a = 1;
void loop() {
  if (buton_1.isPressed()) {
    Serial.print("Pressed: ");
    Serial.println(buton_1.getPin());
  }

  if (buton_1.isReleased() == 1) {
    Serial.print("Released: ");
    Serial.println(buton_1.getPin());
    buton_1.released = -1;
  }

  if (buton_1.isPressedFor(2000)) {
    Serial.print("Pressed ");
    Serial.print(buton_1.getPin());
    Serial.println(" for > 2s");
    buton_1.released = -1;
  }

  if (buton_2.isPressed()) {
    Serial.print("Pressed: ");
    Serial.println(buton_2.getPin());
  }


  if (buton_2.isReleased() == 1) {
    Serial.print("Released: ");
    Serial.println(buton_2.getPin());
    buton_2.released = -1;
  }

  if (buton_2.isPressedFor(2000)) {
    Serial.print("Pressed ");
    Serial.print(buton_2.getPin());
    Serial.println(" for > 2s");
    buton_2.released = -1;
  }

  if (buton_3.isPressed()) {
    Serial.print("Pressed: ");
    Serial.println(buton_3.getPin());
  }


  if (buton_3.isReleased() == 1) {
    Serial.print("Released: ");
    Serial.println(buton_3.getPin());
    buton_3.released = -1;
  }

  if (buton_3.isPressedFor(2000)) {
    Serial.print("Pressed ");
    Serial.print(buton_3.getPin());
    Serial.println(" for > 2s");
    buton_3.released = -1;
  }


}

uint32_t tmp = 0;
ISR(TIMER1_OVF_vect) {
  cnt1++;

  //check debounce every 5ms
  if ((cnt1 % (int)(0.005 / (1.0 / (float)1000))) == 0) {
    buton_1.Debounce();
    buton_2.Debounce();
    buton_3.Debounce();
  }
}
