int buttonPin = A0;
int ledPin = 5;
unsigned long time = 0;
unsigned long startTime = 0;
unsigned long pressedSec = 0;
float periodPot = 10;
float maxSteps = 10;
int previousStep = 0;
int currentStep = 0;
float stepPeriod = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(buttonPin,INPUT);
  pinMode(ledPin,OUTPUT);
  stepPeriod = periodPot / maxSteps;
}

void loop() {
  if(digitalRead(buttonPin) == HIGH){
  }
  else{
     // time=millis();
  }
}
