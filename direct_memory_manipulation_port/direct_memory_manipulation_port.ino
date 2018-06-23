void setup() {
  Serial.begin(9600);
  char buffer[200];
  sprintf(buffer,"PINB:0x%x\tDDRB:0x%x\tPORTB:0x%x",&PINB,&DDRB,&PORTB);
  Serial.println(buffer);
  sprintf(buffer,"PINC:0x%x\tDDRC:0x%x\tPORTC:0x%x",&PINC,&DDRC,&PORTC);
  Serial.println(buffer);
  sprintf(buffer,"PIND:0x%x\tDDRD:0x%x\tPORTD:0x%x",&PIND,&DDRD,&PORTD);
  Serial.println(buffer);

  //pinMode(13, OUTPUT);
  *(char *)(0x24) |= (1<<5);
}

void loop() {
  //digitalWrite(13,HIGH);
  *(char *)(0x25) |= (1<<5);
  delay(500);
  //digitalWrite(13,LOW);
  *(char *)(0x25) &= ~(1<<5);
  delay(500);

}
