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

int didi(int x, int y){
  int a = 1;
  int b = 3;
  uint8_t xl = *((uint8_t *)0x5D);
  uint8_t xh = *((uint8_t *)0x5E);
  uint16_t xsp = *((uint16_t *)0x5D);
  char buffer[200];
  sprintf(buffer,"\t\t%x",xsp);
  Serial.println(buffer);
  
}

void loop() {
  char buffer[200];
  
  uint16_t xsp = *((uint16_t *)&SPL);
  
  sprintf(buffer,"Before:%x",xsp);
  Serial.println(buffer);
  didi(5,6);
  xsp = *((uint16_t *)&SPL);
  sprintf(buffer,"After:%x",xsp);
  Serial.println(buffer);
  xsp = *((uint16_t *)&SPL);
  sprintf(buffer,"After:%x",&PC);
  Serial.println(buffer);
  //digitalWrite(13,HIGH);
  *(char *)(0x25) |= (1<<5);
  delay(500);
  //digitalWrite(13,LOW);
  *(char *)(0x25) &= ~(1<<5);
  delay(3000);
}
