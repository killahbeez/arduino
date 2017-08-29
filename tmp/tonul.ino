  int y = 0;
int z = 0;
int w = 0;
void setup() {

  DDRA |= 1<<A7;
}

void loop() {
  PORTA ^= 1<<A7;
  delay(1);
}
