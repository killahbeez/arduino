uint8_t cntP = 0;
uint8_t cntR = 0;

void setup() {
  Serial.begin(9600);
  TCCR2A |= (1 << WGM20) | (1 << WGM21);
  TCCR2B  |= (1 << CS22) | (1 << CS21) | (1 << CS20) | (1 << WGM22); // Normal port operation, 1024 prescaller, Fast PWM OCRA
  OCR2A = 156; // 100Hz (10ms period)
  TIMSK2 |= (1 << TOIE2);

  DDRB &= ~(1 << PB0); //set PB0 as INPUT
  PORTB |= (1 << PB0); //set INPUT PULLUP resistor
}

void loop() {
  if (buttonPressed(PB0)) {
    cntP++;
    Serial.print("Pressed: ");
    Serial.println(cntP);
  }

  if (buttonReleased(PB0)) {
    cntR++;
    Serial.print("Released: ");
    Serial.println(cntR);
  }

}

ISR(TIMER2_OVF_vect) {
}

boolean buttonPressed(uint8_t pin) {
  static boolean isPushed = 0;
  static boolean lastState = 0;

  isPushed = ( ( PINB & (1 << pin) ) == 0);
  delay(10);
  if (isPushed && !lastState) { //if was pressed
    lastState = isPushed;
    return true;
  }

  if (!isPushed && lastState) { //if released
    lastState = isPushed;
  }


  return false;
}


boolean buttonReleased(uint8_t pin) {
  static boolean isPushed = 0;
  static boolean lastState = 0;

  isPushed = ( ( PINB & (1 << pin) ) == 0);
  delay(10);

  if (!isPushed && lastState) { //if released
    lastState = isPushed;
    return true;
  }

  if (isPushed && !lastState) { //if was pressed
    lastState = isPushed;
  }

  return false;
}

