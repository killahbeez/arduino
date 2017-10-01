/*
Buttons.cpp - Class file for dealing with debounced buttons

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

#include <Buttons.h>

// params 
//		uint8_t pin - expected arduino IDE pin (ex: 8 -> PB0, 14 -> PC0)
//
Buttons::Buttons(uint8_t pin) {
  _pin = pin;

  //setting IO pin as INPUT with PULL UP resistor set
  _DDR(_pin) &= ~digitalPinToBitMask(_pin);
  _PORT(_pin) |= digitalPinToBitMask(_pin);
}

boolean Buttons::isPressed() {
  if (pressed) {
    //pressed = 0;
    return true;
  }

  return false;
}

boolean Buttons::isPressedFor(uint32_t ms) {
  if (isReleased() == 0 && (millis() - _millis_pressed) > ms) {
	pressedFor = 1;
    return true;
  }
  return false;
}

int8_t Buttons::isReleased() {
  if (released == 1) {
    return true;
  }

  return released;
}

boolean Buttons::isClicked() {
	if(isReleased() == 1 && !pressedFor){
		released = -1;
		return true;
	}
	return false;
}

uint8_t Buttons::getPin() {
  return _pin;
}

void Buttons::Debounce() {
  _isPushed = ( (_PIN(_pin) & digitalPinToBitMask(_pin) ) == 0);
  //check if button was pressed
  if (_isPushed && !_lastState) { // if pressed
    _count++;
    // if button has not bounce for 4 checks, the button is debounced and considered pressed
    if (_count >= 4) {
      _lastState = _isPushed;
      pressed = 1;
      released = 0;
	  pressedFor = 0;

      _millis_pressed = millis();
      _count = 0;
      first_press = 1;
    }
  }
  else if (!_isPushed && _lastState) { //if released
    _lastState = _isPushed;
    _millis_released = millis();
    pressed = 0;
	released = 1;
	if(pressedFor){
		released = -1;
	}
    _count = 0;
  }
  else if (!_isPushed) {
    _count = 0;
  }

}