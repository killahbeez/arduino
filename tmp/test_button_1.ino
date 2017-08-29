byte speedUp = 0; //analog 
byte speedDown = 1; //analog 
byte SWButton = 2; //analog 
byte dirButton = 3; //analog 
byte dirPin = 4; //analog

byte speedLedFW = 5; //digital 
byte speedLedRW = 6; //digital
byte enablePin = 7; //digital
byte ledOnOff = 8; //digital

byte crt_speedLed = 5;

unsigned long time = 0;
unsigned long startTime = 0;
unsigned long pressedSec = 0;

float periodPot = 3; //cate secunde pentru a ajunge la maxim pornind de la 0
float maxSteps = 100; // in cati pasi se va ajunge la maxim (numarul de viteze)
float stepPeriod = 0; // periodPot/maxSteps

int currentStep = 0; //step-ul care este :)
int lastStep = 0; //unde a ramas de la ultima setare

void setup() {
  //Serial.begin(9600);
  bitClear(DDRA, 0); //pinMode(speedUpButton, INPUT)
  bitClear(DDRA, 1); //pinMode(speedDown, INPUT)
  bitClear(DDRA, 2); //pinMode(SWButton, INPUT)
  bitClear(DDRA, 3); //pinMode(SWButton, INPUT)

  bitSet(DDRA, 7); //pinMode(enablePin, OUTPUT)
  bitSet(DDRA, 5); //pinMode(speedLedFW, OUTPUT)
  bitSet(DDRA, 6); //pinMode(speedLedRW, OUTPUT)
  
  bitSet(DDRA, 4); //pinMode(dirPin, OUTPUT)
  bitSet(DDRB, 2); //pinMode(ledOnOff, OUTPUT)

  digitalWrite(SWButton, LOW);
  digitalWrite(ledOnOff, LOW);
  stepPeriod = (periodPot / maxSteps); // cat trebuie tinut apasat butonul pentru a insemna o incrementare a pasului
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
        //calculeaza tinand cont de unde s-a oprit, la cati pasi a ajuns de cand e apasat butonul
        int elapsedSteps = (int) ( ((millis() - time) + (lastStep * stepPeriod * 1000)) / (stepPeriod * 1000));

        //daca pasul este mai mic decat maximul configurat incrementeaza
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
        //calculeaza tinand cont de unde s-a oprit, la cati pasi a ajuns de cand e apasat butonul
        int elapsedSteps = (int) ( (millis() - time)  / (stepPeriod * 1000) );

        //daca pasul este mai mic decat maximul configurat incrementeaza
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

  if (digitalRead(SWButton) == HIGH && !previousStateSWB && !inDebounce) {
    lastDebounceTime = millis();
    inDebounce = true;
  }

  if (digitalRead(SWButton) == HIGH && !previousStateSWB && (millis() - lastDebounceTime) > debounceDelay) {
    previousStateSWB = true;
    lastSWB = !lastSWB;

    if (lastSWB) {
      analogWrite(enablePin, (255 / maxSteps)*currentStep);
      analogWrite(crt_speedLed, (255 / maxSteps)*currentStep);
      digitalWrite(ledOnOff,HIGH);
    }
    else {
      digitalWrite(enablePin, LOW);
      digitalWrite(crt_speedLed, LOW);
      digitalWrite(ledOnOff,LOW);
    }

    inDebounce = false;
  }

  if (digitalRead(SWButton) == LOW && previousStateSWB) {
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
      //adu-l treptat la zero inainte de a schimba directia
      for(int i=((255 / maxSteps)*currentStep);i>0;i--){
        analogWrite(enablePin,i);
        analogWrite(speedLedFW,i);
        delay(10);
      }
      digitalWrite(dirPin, HIGH);
      
      //change color of bicolor Led 
      crt_speedLed = speedLedRW;
      digitalWrite(speedLedFW,LOW);
      
      //adu-l treptat la valoarea initiala pe directia schimbata
      for(int i=0;i<((255 / maxSteps)*currentStep);i++){
        analogWrite(enablePin,i);
        analogWrite(crt_speedLed,i);
        delay(10);
      }
      
      analogWrite(crt_speedLed,(255 / maxSteps)*currentStep);
    }
    else {
      //adu-l treptat la zero inainte de a schimba directia
      for(int i=((255 / maxSteps)*currentStep);i>0;i--){
        analogWrite(enablePin,i);
        analogWrite(speedLedRW,i);
        delay(10);
      }
      digitalWrite(dirPin, LOW);
      //change color of bicolor Led 
      crt_speedLed = speedLedFW;
      digitalWrite(speedLedRW,LOW);
      
      //adu-l treptat la valoarea initiala pe directia schimbata
      for(int i=0;i<((255 / maxSteps)*currentStep);i++){
        analogWrite(enablePin,i);
        analogWrite(crt_speedLed,i);
        delay(10);
      }
      
      analogWrite(crt_speedLed,(255 / maxSteps)*currentStep);
    }
    
    
    analogWrite(enablePin, (255 / maxSteps)*currentStep);
    //Serial.println((String) last);
  }

  if (digitalRead(dirButton) == LOW && previousState) {
    previousState = false;
  }

  return last;
}

void blinkLed(byte led) {
  for (byte i = 0; i < 4; i++) {
    analogWrite(led, 10);
    delay(30);
    digitalWrite(led, HIGH);
    delay(30);
  }
}
