void Cacaolu(uint8_t,String);
class Didi
{
    public:
        Didi(){}
        Didi(uint8_t pin){
          _pin = pin;
        }
        uint8_t read(){
          return _pin;
        }
    
    private:
        uint8_t _pin = 7;           //arduino pin number
         uint8_t _i; 
};


Didi marmota1;
Didi marmota2(32);

void setup() {
  Serial.begin(9600);
  Serial.println(marmota1.read());
  Serial.println(marmota2.read());
    //Serial.println(*portInputRegister(digitalPinToPort(PD3)),BIN);
  //Serial.println(PIND,BIN);

}

void loop() {
  // put your main code here, to run repeatedly:

}

void Cacaolu(){
  
}

