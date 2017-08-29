void setup() {
  Serial.begin(115200);
}

void loop() {
  
  while (Serial.available() > 0) {
    uint8_t rcv = (uint8_t)Serial.read();
    char buffer[20];
    sprintf(buffer,"ESP: %d * 2 = %d",rcv,rcv*2);
    Serial.print(buffer);
  }

}
