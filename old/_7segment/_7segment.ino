uint8_t digits[10] = {
  0b11111100, //0
  0b01100000, //1
  0b11011010, //2
  0b11110010, //3
  0b01100110, //4
  0b10110110, //5
  0b10111110, //6
  0b11100000, //7
  0b11111110, //8
  0b11100110  //9
};

#define DS PC0
#define SHCP PC1
#define STCP PC2

void setup() {
  DDRC |= (1 << DS) | (1 << SHCP) | (1 << STCP);
  resetClock();
}

void loop() {
  uint16_t bDigit = 0;
  
  //for (uint8_t i = 0; i <= 60; i++) {
    writeDigits(1);
    //delay(1000);
  //}

 while (1);
}

void writeDigits(uint16_t number) {
  //uint16_t bDigit = getBinaryDigits(number);
  uint8_t crt_bit = 0;
  PORTC &= ~(1 << STCP);
  
  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
    crt_bit = (number >> i) & 1;
    
    if (crt_bit == 1) {
      PORTC |= (1 << DS);
    }
    else {
      PORTC &= ~(1 << DS);
    }

    PORTC |= (1 << SHCP);
  }
  PORTC |= (1 << STCP);
}

uint16_t getBinaryDigits(uint8_t number) {
  //get digits from number
  uint8_t firstdigit;
  uint8_t seconddigit;
  uint16_t compound;

  seconddigit = number % 10;
  firstdigit = number / 10;

  if (firstdigit == 0) {
    compound = digits[seconddigit]<<8;
 }
  else {
   compound = digits[seconddigit] << 8 | digits[firstdigit];
  }

  return compound;
}

void resetClock(){
  PORTC &= ~(1 << STCP);
  
  for (uint8_t i = 0; i < 16; i++) {
    PORTC &= ~(1 << SHCP);
      PORTC &= ~(1 << DS);
    PORTC |= (1 << SHCP);
  }
  PORTC |= (1 << STCP);
  
}
