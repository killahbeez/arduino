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
#include <LiquidTWI2.h>

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

LiquidTWI2 lcd(0x20);
volatile boolean refresh = false;
volatile boolean enter = false;
String buff = "";

void setup() {
  Serial.begin(9600);
  keypad.begin();
  keypad.addEventListener(keypadEvent_didi);

  lcd.setMCPType(LTI_TYPE_MCP23008);

  lcd.begin(16, 2);
  lcd.clear();

}

void loop() {
 keypad.getKey();

  if (refresh) {
    Serial.println("cacat");
    lcd.setCursor(0, 0);
    lcd.print(buff);
    refresh = false;
  }
  if (enter) {
    Serial.println("pisat");
    LCDclearLine(0);
    LCDclearLine(1);
    lcd.setCursor(0, 1);
    lcd.print(buff);
    buff = "";
    enter = false;
  }
  
}

void keypadEvent_didi(KeypadEvent key) {
  switch (keypad.getState()) {
    case PRESSED: {
        if (key == '#')  {
          enter = true;
          Serial.println("");
        }
        else {
          Serial.print(key);
          buff = String(buff + key);
        }
        refresh = true;
        break;
      }
  }
}

void LCDclearLine(uint8_t i) {
  lcd.setCursor(0, i);
  lcd.print("                ");
}

