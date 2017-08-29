/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

int RECV_PIN = 6;

IRrecv irrecv(RECV_PIN);

decode_results results;

uint32_t ircomm = 0;
uint32_t last_ircomm = 0;
uint32_t last_mil = 0;
uint16_t timelapsed = 100;
volatile uint16_t DIM_LIGHT = 0;
uint16_t last_DIM_LIGHT = 0;
uint32_t pushed_lamp_1 = 0;
volatile uint16_t SPEED_FAN = 0;
uint16_t last_SPEED_FAN = 0;
uint32_t pushed_lamp_2 = 0;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  ircomm = 0;
  if (irrecv.decode(&results)) {
    ircomm = results.value;
    //Serial.print(ircomm,HEX);
   //Serial.print("\n");
    if(ircomm != 0xFFFFFFFF && ircomm != 0){
      last_ircomm = ircomm;
    }
    irrecv.resume();
  }
    IRdim(0xFD8877, 0xFD9867, DIM_LIGHT, pushed_lamp_1);
    IRdim(0xFD28D7, 0xFD6897, SPEED_FAN, pushed_lamp_2);
}

void IRdim(uint32_t up, uint32_t down, volatile uint16_t &dim, uint32_t &start_press) {
  static uint32_t last_mil = 0;
  static uint32_t accel_calup = 0;
  static uint16_t timelapsed = 200;

  if ((last_ircomm == up || last_ircomm == down) && ircomm != 0) {

    if (millis() - last_mil > timelapsed) {
      if (last_ircomm == up && dim <= 253) {
        dim+=2;
        start_press++;
      }
      if (last_ircomm == down && dim >= 2) {
        dim-=2;
        start_press++;
      }
      last_mil = millis();

    Serial.print(last_ircomm,HEX);
   Serial.print("\n");
      if (++start_press % 2 == 0) {
        accel_calup = ((uint32_t) (start_press / 2)) + 1;
        if ( (int32_t)(timelapsed - accel_calup * 3) > 0) {
          timelapsed -= accel_calup * 3;
        }
      }
    }
  }
  
  if ( (millis() - last_mil) > (timelapsed + 200)) {
    timelapsed = 200;
    start_press = 0;
  }

}
