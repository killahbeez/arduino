void didi(char *didi1){
  strcpy(didi1,"caca1234");
}
void setup() {
  
   uint8_t f = 123;
  Serial.begin(115200);
   Serial.println(*&f);
   char *muie = "treytrye";
   didi(muie);
   Serial.println(muie);
  char *str = (char*) malloc(16);
  str = "muie la dezinte";
  uint32_t addr = (uint32_t) str;
  Serial.println(addr);
  for(;*str;str++){
    Serial.print((int)str);
    Serial.print("\t=\t");
    Serial.println(*str);
  }
  Serial.println((int)str);
  str = (char*) addr;
  Serial.println(str);
  free(str);
}

void loop() {
}
