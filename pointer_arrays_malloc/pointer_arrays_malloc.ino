uint8_t size = 8;
uint8_t *a = (uint8_t *) malloc(size * sizeof(size));
void setup() {
  Serial.begin(115200);
  struct {
     uint8_t num = 100;
  } var;
  Serial.println(var.num);
  char buffer1[100];
  char buffer2[] = "didilea";
  sprintf(buffer1,"%c",1[buffer2]);
  Serial.println(buffer1);
  sprintf(buffer1,"%p",a);
  Serial.println(buffer1);
  sprintf(buffer1,"%p",&a[0]);
  Serial.println(buffer1);
  
  Serial.println((int)&a[0]);
  Serial.println((int) & (*a));

  for (uint8_t i = 0; i < size; i++ ) {
    *(a + i) = i * 2;
  }
  char buffer[20];
  sprintf(buffer,"%d",(int)*(&(*(a+5))));
  Serial.println(buffer);
  Serial.println("_____________");
  for (uint8_t i = 0; i < size; i++ ) {
    Serial.print((int) & (*(a + i)));
    Serial.print(" = ");
    Serial.println(*(a + i));
  }

  uint8_t b[2] = {1, 2};
  
  sprintf(buffer1,"%p",b);
  Serial.println(buffer1);
  sprintf(buffer1,"%x",&*b);
  Serial.println(buffer1);
  Serial.println(sizeof(uint32_t*));
  Serial.println(sizeof(uint32_t));
  Serial.println("__________");
  didi(b);
  Serial.println(b[0]);
  Serial.println(b[1]);
  Serial.println(a[4]);
  dexter(&(*(a + 4))); // address for a[4]
  dexter(&(*(a + 5))); // address for a[5]
  Serial.println(a[4]);

  for (uint8_t i = 0; i < size; i++ ) {
    Serial.print((int) & (*(a + i)));
    Serial.print(" = ");
    Serial.println(*(a + i));
  }

}

void loop() {
  // put your main code here, to run repeatedly:

}

void didi(uint8_t a[]) {
  a[0] = 53;
  a[1] = 54;
}

void dexter(uint8_t *tmp) {
  *tmp = 123;
}

