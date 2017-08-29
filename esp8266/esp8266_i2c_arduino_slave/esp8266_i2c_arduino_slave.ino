#include <Wire.h>
#define I2CAddressESPWifi 16

void setup() {
  Serial.begin(115200);
  Wire.begin(I2CAddressESPWifi);
  Wire.onReceive(espWifiReceiveEvent);

}

void loop() {
  // put your main code here, to run repeatedly:

}

void espWifiReceiveEvent(int count)
{
  while (Wire.available())
  {
    char c = Wire.read();
    Serial.print(c);
  }
  Serial.println("");
}
