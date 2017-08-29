/* @file HelloKeypad.pde
|| @version 1.0
|| @author Alexander Brevig
|| @contact alexanderbrevig@gmail.com
||
|| @description
|| | Demonstrates the simplest use of the matrix Keypad library.
|| #
   Modified for Keypad_MCP G. D. (Joe) Young July 29/12
*/
#include <Keypad_MCP.h>
#include <Wire.h>
#include <Keypad.h>

#define I2CADDR 0x21

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {0, 1, 2, 3}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 5, 6, 7}; //connect to the column pinouts of the keypad

Keypad_MCP keypad = Keypad_MCP( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR );

void setup() {
  Serial.begin(9600);
  keypad.begin( );
  keypad.addEventListener(keypadEvent_didi);
}

void loop() {
  keypad.getKey();
}

void keypadEvent_didi(KeypadEvent key) {
  switch (keypad.getState()) {
    case PRESSED: {
        if (key == '#')  {
          Serial.println("");
        }
        else {
          Serial.print(key);
        }
        break;
      }
  }
}

