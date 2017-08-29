#define speedUp      0 //PA0 analog
#define speedDown    1 //PA1 analog
#define SWButton     2 //PA2 analog
#define dirButton    3 //PA3 analog
#define dirPin       4 //PA4 analog

#define speedLedFW   5 //PA5 digital
#define speedLedRW   6 //PA6 digital
#define enablePin    7 //PA7 digital
#define ledOnOff     8 //PB2 digital

uint8_t crt_speedLed = 5; //PA5

#define BV(x)  (1<<x)
#define setBit(P,B)  P |= BV(B)  
#define clearBit(P,B)  P &= ~BV(B)  
#define toggleBit(P,B)  P ^= BV(B)  

unsigned long time = 0;
unsigned long startTime = 0;
unsigned long pressedSec = 0;

float periodPot = 3; //how many seconds to reach maximum speed starting from 0
float maxSteps = 100; //in how many steps will reach maximum speed (how many speeds)
float stepPeriod = 0; // periodPot/maxSteps

int currentStep = 0; //the current step :)
int lastStep = 0; //where it is from the last setting

void setup() {
  //TCCR0B = TCCR0B & 0b11111000 | 1<<1;
  //Serial.begin(9600);
  DDRA = BV(PA7) | BV(PA6) | BV(PA5) | BV(PA4); //sets enablePin + speedLedRW + speedLedFW + dirPin as OUTPUT
  DDRB = BV(PB2); //sets ledOnOff as OUTPUT

  clearBit(PORTA,PA2);//sets SWButton as LOW
  clearBit(PORTB,PB2);//sets ledOnOff as LOW
  
  stepPeriod = (periodPot / maxSteps); // how much time should the button be pressed to be count as one step
}

void loop() {
  if (isOn_SWButton()) {
    isOn_DirButton();
    if (!pressedSpeedUp() && !pressedSpeedDown()) {
      time = millis();

      if (currentStep > maxSteps) {
        currentStep = maxSteps;
      }

      if (currentStep < 0) {
        currentStep = 0;
      }

      lastStep = currentStep;
    }

    if (currentStep >= 0 && currentStep <= maxSteps && !(pressedSpeedUp() && pressedSpeedDown()) ) {
      if (pressedSpeedUp()) {
        //calculate from the last set, how many steps are reached after the button is pressed
        int elapsedSteps = (int) ( ((millis() - time) + (lastStep * stepPeriod * 1000)) / (stepPeriod * 1000));

        //if the current step is less than maximum set it up
        if (currentStep < maxSteps &&  elapsedSteps > currentStep) {
          currentStep = elapsedSteps;

          if (currentStep == maxSteps) {
            blinkLed(crt_speedLed);
          }

          //Serial.print((String)(int)((millis() - time) / 1000) + " sec ");
          //Serial.println(currentStep);
        }

        analogWrite(enablePin, (255 / maxSteps)*currentStep);
        analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
      }

      if (pressedSpeedDown()) {
        //calculate from the last set, how many steps are reached after the button is pressed
        int elapsedSteps = (int) ( (millis() - time)  / (stepPeriod * 1000) );

        //if the current step is higher than 0, make the difference
        if (currentStep > 0 && (lastStep - elapsedSteps) < currentStep && (lastStep - elapsedSteps) >= 0) {
          currentStep = lastStep - elapsedSteps;

          if (currentStep == 0) {
            blinkLed(crt_speedLed);
          }

          //Serial.print((String)(int)((millis() - time) / 1000) + " sec ");
          //Serial.println(currentStep);
        }

        analogWrite(enablePin, (255 / maxSteps)*currentStep);
        analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
      }
    }
  }
}

boolean pressedSpeedUp() {
  return digitalRead(speedUp);
}

boolean pressedSpeedDown() {
  return digitalRead(speedDown);
}

boolean isOn_SWButton() {
  static boolean inDebounce = false;
  static boolean lastSWB = false;
  static boolean previousStateSWB = false;
  static int lastDebounceTime = 0;
  static int debounceDelay = 100;

  if ((PINA & (1<<PA2)) && !previousStateSWB && !inDebounce) {
    lastDebounceTime = millis();
    inDebounce = true;
  }

  if ((PINA & (1<<PA2)) && !previousStateSWB && (millis() - lastDebounceTime) > debounceDelay) {
    previousStateSWB = true;
    lastSWB = !lastSWB;

    if (lastSWB) {
      digitalWrite(ledOnOff, HIGH);
      slowMotion(true, crt_speedLed, 4);
      analogWrite(enablePin, (255 / maxSteps)*currentStep);
      analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
    }
    else {
      slowMotion(false, crt_speedLed, 4);
      digitalWrite(enablePin, LOW);
      digitalWrite(crt_speedLed, LOW);
      digitalWrite(ledOnOff, LOW);
    }

    inDebounce = false;
  }

  if ((PINA & (1<<PA2)) == 0 && previousStateSWB) {
    previousStateSWB = false;
  }

  return lastSWB;
}

boolean isOn_DirButton() {
  static boolean inDebounce = false;
  static boolean last = false;
  static boolean previousState = false;
  static int lastDebounceTime = 0;
  static int debounceDelay = 100;

  if (digitalRead(dirButton) == HIGH && !previousState && !inDebounce) {
    lastDebounceTime = millis();
    inDebounce = true;
  }

  if (digitalRead(dirButton) == HIGH && !previousState && (millis() - lastDebounceTime) > debounceDelay) {
    previousState = true;
    last = !last;
    inDebounce = false;

    if (last) {
      slowMotion(false, speedLedFW, 4);
      digitalWrite(dirPin, HIGH);

      //change color of bicolor Led
      crt_speedLed = speedLedRW;
      digitalWrite(speedLedFW, LOW);

      slowMotion(true, crt_speedLed, 4);
      analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
    }
    else {

      slowMotion(false, speedLedRW, 4);
      digitalWrite(dirPin, LOW);

      //change color of bicolor Led
      crt_speedLed = speedLedFW;
      digitalWrite(speedLedRW, LOW);

      slowMotion(true, crt_speedLed, 4);
      analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
    }


    analogWrite(enablePin, (255 / maxSteps)*currentStep);
    //Serial.println((String) last);
  }

  if (digitalRead(dirButton) == LOW && previousState) {
    previousState = false;
  }

  return last;
}

void slowMotion(boolean isForward, byte speedLed, int delay_time) {
  switch (isForward) {
    case true:
      //bring speed slowly to set value
      for (int i = 0; i < ((255 / maxSteps)*currentStep); i++) {
        analogWrite(enablePin, i);
        analogWrite(speedLed, i);
        delay(delay_time);
      }
      
      if (currentStep == maxSteps) {
        blinkLed(speedLed);
      }
      break;
    default:
      //bring speed slowly to zero
      for (int i = ((255 / maxSteps) * currentStep); i > 0; i--) {
        analogWrite(enablePin, i);
        analogWrite(speedLed, i);
        delay(delay_time);
      }

  }
}

void blinkLed(byte led) {
  for (byte i = 0; i < 4; i++) {
    analogWrite(led, 10);
    delay(30);
    digitalWrite(led, HIGH);
    delay(30);
  }
}
