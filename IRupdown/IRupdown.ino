/*
 * IRremote: IRrecvDemo - demonstrates receiving IR codes with IRrecv
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * http://arcfn.com
 */

#include <IRremote.h>

uint8_t RECV_PIN = 6;
uint16_t SPEED_FAN = 0;
uint16_t last_SPEED_FAN = 0;

IRrecv irrecv(RECV_PIN);

decode_results results;

void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver
}

void loop() {
  last_SPEED_FAN = SPEED_FAN;
  IRdim(0x490, 0xC90, SPEED_FAN);
  if (last_SPEED_FAN != SPEED_FAN) {
    Serial.println(SPEED_FAN);
  }
}

void IRdim(uint32_t up, uint32_t down, uint16_t &dim) {
  static uint32_t ircomm = 0;
  static uint32_t last_millis = 0;

  static uint32_t start_press = 0;
  static uint32_t accel_calup = 0;
  static uint16_t timelapsed = 200;

  if (irrecv.decode(&results)) {
    ircomm = results.value;

    if (millis() - last_millis > timelapsed) {
      if (ircomm == up && dim < 255) {
        dim++;
        start_press++;
      }
      if (ircomm == down && dim > 0) {
        dim--;
        start_press++;
      }
      last_millis = millis();

      if (++start_press % 2 == 0) {
        accel_calup = ((uint32_t) (start_press / 2)) + 1;
        if ( (int32_t)(timelapsed - accel_calup * 3) > 0) {
          timelapsed -= accel_calup * 3;
        }
      }
      //Serial.print("     ");
      //Serial.println(timelapsed);
    }
    irrecv.resume();
  }
  else if ( (millis() - last_millis) > (timelapsed + 100)) {
    timelapsed = 200;
    start_press = 0;
  }
}

