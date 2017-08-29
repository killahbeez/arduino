#include <IRremote.h>
int RECV_PIN = 13;
IRrecv irrecv(RECV_PIN);
decode_results results;

uint32_t ircomm = 0;
uint32_t last_ircomm = 0;

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
  // put your setup code here, to run once:

}

void loop() {
  ircomm = 0;
  if (irrecv.decode(&results)) {
    ircomm = results.value;
    if (ircomm != 0xFFFFFFFF && ircomm != 0) {
      last_ircomm = ircomm;
    }
    irrecv.resume();
  Serial.print(ircomm,HEX);
  Serial.println("");
  }

}
