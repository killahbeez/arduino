/*
Buttons.h - Header file for dealing with debounced buttons

Version: 1.0.0
(c) 2016 Adrian Carstea

This program is free software: you can redistribute it and/or modify
it under the terms of the version 3 GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "Arduino.h"

#define _DDR(P) *portModeRegister(digitalPinToPort(P))
#define _PORT(P) *portOutputRegister(digitalPinToPort(P))
#define _PIN(P) *portInputRegister(digitalPinToPort(P))

#ifndef Buttons_h
#define Buttons_h

class Buttons {
  public:
    boolean pressed = 0;
    int8_t released = -1;
    boolean first_press = 1;

    Buttons(uint8_t pin);

    boolean isPressed();
    int8_t isReleased();
    boolean isPressedFor(uint32_t ms);
    boolean isClicked();
    void Debounce();
    uint8_t getPin();

  private:
    uint8_t _pin;
    boolean _isPushed = 0;
    uint8_t _lastState = 0;
    uint8_t _count = 0;
    uint32_t _millis_pressed;
    uint32_t _millis_released;
	boolean pressedFor = 0;

};

#endif