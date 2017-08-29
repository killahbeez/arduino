
#include <SoftwareSerial.h>

SoftwareSerial mySerial(10, 11); // RX, TX

void setup() {

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  Serial.begin(9600);
  // put your setup code here, to run once:

}

void loop() {
  if (mySerial.available() > 0) {
    while (mySerial.available() > 0) {
      Serial.print((char)mySerial.read());
    }
    
  }
  // put your main code here, to run repeatedly:

}
