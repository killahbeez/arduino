
/*

Read pot on ANC1 convert value to voltage
then display on Arduino serial monitor.

By Lewis Loflin lewis@bvu.net
http://www.sullivan-county.com/main.htm
Electronics website:
http://www.bristolwatch.com/index.htm

*/

#include <Wire.h> // specify use of Wire.h library
#define ASD1115 0x48

unsigned int val = 0;
byte buffer[3];

const float VPS = 6.144 / 32768; // volts per step
float voltage = 0;
uint16_t a = 0;

void setup()   {

  Serial.begin(9600);
  Wire.begin(); // begin I2C
  delay(500);

}  // end setup

uint8_t b = 0;
uint8_t b1 = 0;
uint8_t b2 = 0;
uint8_t b3 = 0;

uint8_t didi[] = {0b11000001, 0b11010001, 0b11100001, 0b11110001};

void loop() {

  for (uint8_t i = 0; i < 4; i++) {
    val = 0;
    Wire.beginTransmission(ASD1115);  // ADC
    Wire.write(0b00000001);
    Wire.write(didi[i]);
    Wire.write(0b10000011);
    Wire.endTransmission();
    delay(8);

    buffer[0] = 0; // pointer
    Wire.beginTransmission(ASD1115);  // DAC
    Wire.write(0b00000000);  // pointer
    Wire.endTransmission();

    Wire.requestFrom(ASD1115, 2);
    buffer[1] = Wire.read();  //
    buffer[2] = Wire.read();  //

    // convert display results
    val = buffer[1] << 8 | buffer[2];

    if (val > 32768) val = 0;

    Serial.print(val * VPS, 5);
    Serial.print("\t\t");
  }

  delay(8);
  Serial.println();


} // end loop






