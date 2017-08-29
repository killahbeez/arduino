#include <EEPROM.h>
#include <TEA5767.h>
#include <Wire.h>
#include <Buttons.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

TEA5767 Radio;
int search_mode = 0;
unsigned long last_pressed;
uint8_t cnt_radio_list = 0;

volatile uint32_t cnt1 = 0;
Buttons btn_forward(7); // PD7
Buttons btn_backward(6); // PD6
Buttons btn_enc(16); //PC2

#define ENC_CTL  DDRC  //encoder port control
#define ENC_WR  PORTC //encoder port write  
#define ENC_RD  PINC  //encoder port read

volatile float radio_freq;

int stereo;
int signal_level;
volatile boolean chg_freq = 0;
boolean show_saved_EEPROM = 0;
uint32_t saved_millis = millis();

char* radio_name;

typedef struct {
  float freq;
  char* name;
} radio_detail;

radio_detail list_radio[27] = {
  {88.0, "Radio Impuls"},
  {89.0, "Radio ZU"},
  {89.5, "Dance FM"},
  {90.8, "Magic FM"},
  {91.7, "National FM"},
  {92.1, "Vibe FM"},
  {92.7, "Radio Tananana"},
  {93.5, "RFI Romania"},
  {94.8, "Guerrilla"},
  {96.1, "Kiss FM"},
  {96.9, "Gold FM"},
  {97.9, "DIGI FM"},
  {98.3, "Bucuresti FM"},
  {99.3, "Radio Itsy Bitsy"},
  {99.6, "Digi FM"},
  {100.2, "Radio 21"},
  {100.6, "Rock FM"},
  {101.3, "Romania Cultural"},
  {101.9, "Romantic FM"},
  {102.8, "PROFM"},
  {103.4, "Radio Seven"},
  {103.8, "Music FM"},
  {104.8, "Romania Muzical"},
  {105.3, "Romania Actualitati"},
  {105.8, "Sport Total FM"},
  {106.2, "City FM"},
  {106.7, "Europa FM"}
};


U8G2_PCD8544_84X48_F_4W_SW_SPI u8g(U8G2_R0, /* clock=*/ 8, /* data=*/ 10, /* cs=*/ 11, /* dc=*/ 9, /* reset=*/ 12);  // Nokia 5110 Display

const uint8_t fist[] PROGMEM = {
  0x00, 0x00, 0x00, 0xe0, 0xff, 0x00, 0x10, 0x00, 0x03, 0x08, 0x00, 0x04,
  0x04, 0x00, 0x08, 0x14, 0x12, 0x08, 0x14, 0x02, 0x0a, 0x14, 0x20, 0x0a,
  0x14, 0x24, 0x0a, 0x34, 0x24, 0x0a, 0x32, 0x22, 0x08, 0xe2, 0x23, 0x07,
  0x82, 0xfd, 0x03, 0x02, 0x1e, 0x00, 0x1c, 0x19, 0x00, 0x18, 0x0f, 0x00,
  0xe0, 0x00, 0x00
};

void setup() {
  Wire.begin();
  Radio.init();

  radio_freq = EEP_read(0);
  if (!(radio_freq > 87.6 && radio_freq < 107.9)) {
    radio_freq = 101.9;
  }

  Radio.set_frequency(radio_freq);
  radio_name = showRadioName(radio_freq);
  
  Serial.begin(9600);
  u8g.begin();

  TCCR1A |= (1 << WGM10) | (1 << WGM11);
  TCCR1B = (1 << CS11) | (1 << CS10) | (1 << WGM12) | (1 << WGM13); // Normal port operation, 64 prescaller, Fast PWM OCRA
  OCR1A = 16000000 / 64 / 1000;
  TIMSK1 = (1 << TOIE1);

  //enable encoder pins interrupt sources (A0 A1)
  PCMSK1 |= (( 1 << PCINT8 ) | ( 1 << PCINT9 ));
  PCICR |= ( 1 << PCIE1 );

  DDRD |= (1 << PD6);
  analogWrite(PD6, 150);
  //Serial.println(sizeof(radios_buc)/sizeof(radios_buc[0]));
}

void draw(void) {
  uint32_t current_millis = millis();

  u8g.clearBuffer();          // clear the internal memory

  u8g.drawXBMP( 0, 0, 20, 17, fist);

  u8g.setFont(u8g2_font_chikita_tf);
  u8g.drawStr(25, 10, "Radio by Adi");

  if (show_saved_EEPROM && (millis() - saved_millis) < 1000) {
    u8g.drawDisc(80, 5, 3);
  }
  else {
    show_saved_EEPROM = 0;
  }

  u8g.setFont(u8g2_font_crox2hb_tr);
  char buffer[20];
  dtostrf(radio_freq, 5, 1, buffer);
  sprintf(buffer, "%s Mhz", buffer);
  u8g.drawStr(10, 27, buffer);

  if(radio_name != "NO_NAME"){
    u8g.setFont(u8g2_font_synchronizer_nbp_tf);
    u8g.drawStr(2, 38, radio_name);
  }
  
  u8g.setFont(u8g2_font_micro_mr);
  if (stereo) {
    u8g.drawStr(2, 48, "stereo");
  }
  else {
    u8g.drawStr(2, 48, "mono");
  }

  sprintf(buffer, "%d/15", signal_level);
  u8g.drawStr(65, 48, buffer);
  
  u8g.sendBuffer();          // transfer internal memory to the display
}

void loop() {
  unsigned char buf[5];
  uint32_t current_millis = millis();
  static uint32_t eeprom_millis = millis();
  static int8_t not_saved = -1;

  if (chg_freq) {
    Radio.set_frequency(radio_freq);
    not_saved = 0;
    eeprom_millis = millis();

    radio_name = showRadioName(radio_freq);

    //Serial.print("\n");
    chg_freq = 0;
  }

  // save only after 1 sec of inactivity of encoder
  if ((millis() - eeprom_millis ) > 1000 && not_saved == 0) {
    EEP_write(0, radio_freq);
    show_saved_EEPROM = 1;
    saved_millis = millis();
    not_saved = 1;
    /*Serial.print(radio_freq);
      Serial.println(" saved");*/

  }
  Wire.requestFrom(0x60, 5); //reading TEA5767
  if (Wire.available()) {
    for (int i = 0; i < 5; i++) {
      buf[i] = Wire.read();
    }
    stereo = Radio.stereo(buf);
    signal_level = Radio.signal_level(buf);
  }

  if (btn_enc.isClicked()) {
    cnt_radio_list++;
    if (cnt_radio_list >= sizeof(list_radio) / sizeof(list_radio[0])) {
      cnt_radio_list = 0;
    }
    
    radio_freq =  list_radio[cnt_radio_list].freq;
    chg_freq = 1;

  }

  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );
}

void EEP_write(uint16_t addr, float cnt) {
  uint8_t HB, LB;
  HB = (uint8_t)cnt;
  LB = round(((float)(cnt - HB)) * 10);
  EEPROM.write(addr, HB);
  EEPROM.write(addr + 1, LB);

}

float EEP_read(uint16_t addr) {
  uint8_t HB = EEPROM.read(addr);
  uint8_t LB = EEPROM.read(addr + 1);

  return (float) (HB + (float) LB / 10);
}

char* showRadioName(float freq) {
  for (uint8_t cnt = 0; cnt < sizeof(list_radio) / sizeof(list_radio[0]); cnt++) {    
    if (round(list_radio[cnt].freq * 10) == round(freq * 10)) {
      cnt_radio_list = cnt;
      return list_radio[cnt].name;
    }
  }
  return "NO_NAME";
}

ISR(TIMER1_OVF_vect) {
  cnt1++;

  //check debounce every 5ms
  if ((cnt1 % (int)(0.005 / (1.0 / (float)1000))) == 0) {
    btn_forward.Debounce();
    btn_backward.Debounce();
    btn_enc.Debounce();
  }
}

/* encoder routine. Expects encoder with four state changes between detents */
/* and both pins open on detent */
ISR(PCINT1_vect)
{
  static uint8_t old_AB = 3;  //lookup table index
  static int8_t encval = 0;   //encoder value
  static const int8_t enc_states [] PROGMEM = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0}; //encoder lookup table

  old_AB <<= 2; //remember previous state
  old_AB |= ( ENC_RD & 0x03 );
  encval += pgm_read_byte(&(enc_states[( old_AB & 0x0f )])); //nibble compose by previous state and current 1110 (14) = -1 => 1000 (8) = -1

  //on detent
  if ( encval > 3 ) { //four steps forward
    encval = 0;

    if (radio_freq < 107.9) {
      radio_freq += 0.1;
      chg_freq = 1;
    }
  }
  else if ( encval < -3 ) { //four steps backwards
    encval = 0;

    if (radio_freq > 87.6) {
      radio_freq -= 0.1;
      chg_freq = 1;
    }
  }
}
