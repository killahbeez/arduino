#include <IRremote.h>

int RECEIVE_PIN = 2;
int brightness = 0;
int LED = 10;
IRrecv irrecv(RECEIVE_PIN);
IRsend irsend;
int a = 0;

decode_results results;
boolean blinking = true;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  //irsend.enableIROut(38);
  //Serial.println(TIMER_INTR_NAME);
}

void loop() {
  
  /*for (int i = 0; i < 3; i++) {
      irsend.sendSony(0xa90, 12); // Sony TV power code
      delay(10);
    }
   while(1);
  */
  if (irrecv.decode(&results)) {

    switch (results.value) {
      case 0x490:
        if (brightness < 255) {
          brightness += 5;
          if (brightness > 255) {
            brightness = 255;
          }
          blinking = true;
        }
        else {
          if (blinking) {
            blinking = false;
            for (int i = 0; i < 5; i++) {
              PORTB ^= 1 << PB2;
              delay(50);
            }
            //PORTB = 1 << PB2;
          }
        }
        break;

      case 0xC90:
        if (brightness > 0) {
          brightness -= 5;
          if (brightness < 0) {
            brightness = 0;
          }
          blinking = true;
        }
        else {
          if (blinking) {
            blinking = false;
            for (int i = 0; i < 5; i++) {
              PORTB ^= 1 << PB2;
              delay(50);
            }
            //PORTB &= ~(1 << PB2);
          }
        }
        break;
    }

    analogWrite(LED, brightness);
    irrecv.resume();
  }

}
