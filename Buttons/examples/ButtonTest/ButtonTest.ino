
#include <util/atomic.h>
#include <Buttons.h>

volatile uint32_t cnt1 = 0;

Buttons buton_1(7); // PD7
Buttons buton_2(6); // PD6

uint16_t freq_multiplex;

void setup() {
  Serial.begin(9600);
  
  freq_multiplex = 1000;
  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / freq_multiplex;
  TIMSK1 = (1 << TOIE1);
  Serial.println(OCR1A);
}

char buffer[50];

void loop() {
  /*if (buton_1.isPressed()) {
	sprintf(buffer,"Pressed: %d",buton_1.getPin());
	Serial.println(buffer);
	buton_1.pressed = 0;
  }

  if (buton_1.isReleased() == 1) {
	sprintf(buffer,"Released: %d",buton_1.getPin());
	Serial.println(buffer);
    buton_1.released = -1;
  }*/

  if(buton_1.isClicked()){
	sprintf(buffer,"Clicked: %d",buton_1.getPin());
	Serial.println(buffer);
  }

  if (buton_2.isPressed()) {
	sprintf(buffer,"Pressed: %d",buton_2.getPin());
	Serial.println(buffer);
	buton_2.pressed = 0;
  }
  
  if (buton_2.isReleased() == 1) {
	sprintf(buffer,"Released: %d",buton_2.getPin());
	Serial.println(buffer);
    buton_2.released = -1;
  }

  if(buton_2.isClicked()){
	sprintf(buffer,"Clicked: %d",buton_2.getPin());
	Serial.println(buffer);
  }

  if (buton_1.isPressedFor(3000) && buton_2.isPressedFor(3000)) {
	sprintf(buffer,"Simultan pressed %d %d for more than 3s",buton_1.getPin(),buton_2.getPin());
	Serial.println(buffer);
    buton_1.released = -1;
    buton_2.released = -1;
  }
}

uint32_t tmp = 0;
ISR(TIMER1_OVF_vect) {
  cnt1++;
  
  //check debounce every 5ms
  if ((cnt1 % (int)(0.005 / (1.0 / (float)freq_multiplex))) == 0) {
    buton_1.Debounce();
    buton_2.Debounce();
  }
}