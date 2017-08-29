struct params {
  uint8_t i;
  String txt;
};

params didi;
params dexter;

void test(struct params vals){
  Serial.println(vals.i);
  Serial.println(vals.txt);
}

void setup() {
  Serial.begin(115200);
  didi = {123, "caca maca"};
  dexter = {1,"gigi marga"};
  // put your setup code here, to run once:

}

void loop() {
  test(didi);
  delay(1000);
  test(dexter);
  delay(1000);
  test((params) {345,"muie"});
  while(1);
  // put your main code here, to run repeatedly:

}
