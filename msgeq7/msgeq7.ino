#define leftChannelPin 0
#define rightChannelPin 1
#define strobePin PD2
#define resetPin PD3

uint16_t spectrumLeftValue[7];
uint16_t spectrumRightValue[7];
uint8_t filter = 80;

void setup() {
  Serial.begin(9600);         //needed to output the values of the frequencies bands on the serial monitor
  DDRC &= ~(1 << PC0) & ~(1 << PC1);
  DDRD |= (1 << strobePin) | (1 << resetPin);
  MSGEQ7_reset();
}

uint8_t a = 1;
uint8_t a1 = 1;
uint8_t a2 = 1;
uint8_t a3 = 1;
uint8_t a4 = 1;
uint8_t a31 = 1;
uint8_t a42 = 1;
void loop() {
  //MSGEQ7_reset();
  MSGEQ7_read();
  delay(100);
}


void MSGEQ7_reset() {
  PORTD &= ~(1 << resetPin);  //init with STROBE and RESET low
  PORTD &= ~(1 << strobePin);

  PORTD |= (1 << resetPin) ; //set RESET high
  PORTD |= (1 << strobePin); //set STROBE high
  delayMicroseconds(18); //delay 18us for STROBE high (see timing diagram)

  PORTD &= ~(1 << strobePin); //set STROBE low
  delayMicroseconds(54); //delay 54us (tss 72us) for STROBE low (see timing diagram)

  PORTD &= ~(1 << resetPin); //set RESET low
  PORTD |= (1 << strobePin); //set STROBE high
  delayMicroseconds(18); //delay 18us for STROBE high (see timing diagram)
}


void MSGEQ7_read() {
  for (uint8_t i = 0; i < 7; i++) {
    PORTD &= ~(1 << strobePin); //set STROBE low
    delayMicroseconds(36); // to = 36 us (see timing diagram)
    spectrumLeftValue[i] = analogRead(leftChannelPin);
    spectrumRightValue[i] = analogRead(rightChannelPin);

    if (spectrumLeftValue[i] < filter) {
      //spectrumLeftValue[i] = 0;
    }
    if (spectrumRightValue[i] < filter) {
      //spectrumRightValue[i] = 0;
    }

    //spectrumLeftValue[i] = map(spectrumLeftValue[i], 0, 1023, 0, 255);
    //spectrumRightValue[i] = map(spectrumRightValue[i], 0, 1023, 0, 255);

    PORTD |= (1 << strobePin); //set STROBE high
    delayMicroseconds(36); // tss = 72 us (see timing diagram)

    Serial.print(spectrumLeftValue[i]);
    //Serial.print(":");
    //Serial.print(spectrumRightValue[i]);
    Serial.print(" ");
  }

  Serial.print("\n");
}

