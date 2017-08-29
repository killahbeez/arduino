#include <SPI.h>
#define SSPin PB2
uint8_t didi = 0b00000000;

void setup() {
  // put your setup code here, to run once:
  DDRB |= 1 << SSPin; // Set the SS pin as an output
  PORTB |= 1 << SSPin; // Set the SS pin HIGH
  SPI.begin();  // Begin SPI hardware
  SPI.setClockDivider(SPI_CLOCK_DIV64);  // Slow down SPI clock

  clearLeds();

}


void loop() {
  // put your main code here, to run repeatedly:
  test1(100);
  clearLeds();
  delay(100);
  test2(100);
  clearLeds();
  delay(100);

}

void clearLeds() {
  PORTB &= ~(1 << SSPin);
  didi = 0;
  SPI.transfer(didi);
  PORTB |= (1 << SSPin);
}

void test1(uint8_t delay_ms) {

  for (uint8_t i = 0; i < 8; i++) {
    didi |= (1 << i);
    PORTB &= ~(1 << SSPin);
    SPI.transfer(didi);
    PORTB |= (1 << SSPin);
    delay(delay_ms);
  }

  for (uint8_t i = 0; i < 8; i++) {
    didi = (didi >> 1);
    PORTB &= ~(1 << SSPin);
    SPI.transfer(didi);
    PORTB |= (1 << SSPin);
    delay(delay_ms);
  }
}

void test2(uint8_t delay_ms) {

  for (uint8_t i = 0; i < 8; i++) {
    didi = (1 << i);
    PORTB &= ~(1 << SSPin);
    SPI.transfer(didi);
    PORTB |= (1 << SSPin);
    delay(delay_ms);
  }

  for (uint8_t i = 0; i < 8; i++) {
    didi = (didi >> 1);
    PORTB &= ~(1 << SSPin);
    SPI.transfer(didi);
    PORTB |= (1 << SSPin);
    delay(delay_ms);
  }
}
