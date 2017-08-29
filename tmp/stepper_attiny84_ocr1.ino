
//int16_t set_delay = 0;
//int16_t set_step = 0;
uint8_t current_seq = 0;

float delay_long = 0.00;
volatile boolean manual_move_fwd = false;
volatile boolean manual_move_bck = false;
volatile boolean is_min = false;
volatile boolean is_max = false;

uint32_t OCR1A_temp = 0;

#define STEPPER_PIN_A        PA0
#define STEPPER_PIN_NA       PA1
#define STEPPER_PIN_B        PA2
#define STEPPER_PIN_NB       PA3
#define MANUAL_POT_PIN       PA7

#define STEP_PIN             PA4
#define DIR_PIN              PA5
#define MANUAL_FWD_PIN       PA6
#define MANUAL_BCK_PIN       PB2

#define PCINT_MSK_0          PCMSK0
#define PCINT_MSK_1          PCMSK1
#define PCINT_STEP           PCINT4
#define PCINT_DIR            PCINT5
#define PCINT_MANUAL_FWD     PCINT6
#define PCINT_MANUAL_BCK     PCINT10
#define PCINT_STOP_MIN       PCINT8
#define PCINT_STOP_MAX       PCINT9
#define PIN_CHG_INT_ENABLE_0   PCIE0
#define PIN_CHG_INT_ENABLE_1   PCIE1

#define F_CPU                8000000

volatile boolean is_forward = true; //for manual
volatile uint16_t SPEED_MANUAL = 3; //for manual

volatile uint16_t currentPotValue = 0;
volatile uint16_t lastPotValue = 0;
volatile uint16_t aval = 0;
volatile uint32_t elapsed_sleep_motor = 0;
volatile boolean sleep_motor = false;

int a = 0;
int b = 0;

void setup() {
  //CLKPR = 0x80;
  //CLKPR = 0x01;
  //Serial.begin(9600);
  cli();
  // OUTPUT IN1 + IN2 + IN3 + IN4
  DDRA |= 1 << STEPPER_PIN_A;
  DDRA |= 1 << STEPPER_PIN_NA;
  DDRA |= 1 << STEPPER_PIN_B;
  DDRA |= 1 << STEPPER_PIN_NB;

  // INPUT MANUAL_POT_PIN
  DDRA &= ~(1 << MANUAL_POT_PIN);

  PCINT_MSK_0 |= (1 << PCINT_STEP) | (1 << PCINT_DIR) | (1 << PCINT_MANUAL_FWD);
  PCINT_MSK_1 |=  (1 << PCINT_MANUAL_BCK) | (1 << PCINT_STOP_MIN) | (1 << PCINT_STOP_MAX);
  GIMSK |= (1 << PIN_CHG_INT_ENABLE_0) | (1 << PIN_CHG_INT_ENABLE_1);

  //setting timer 1
  TCCR1A = (1 << WGM10) | (1 << WGM11); // Fast PWM OCR1 TOP, Normal port operation, OC1A/OC1B disconnected.
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS12) ; // FCPU/256 prescaler
  TIMSK1 = (1 << OCIE1B) | (1 << TOIE1); //Timer/Counter1, Output Compare A Match Interrupt Enable + Overflow Interrupt Enable

  //setting ADC
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1);// Set ADC prescalar to 64 – 125KHz sample rate @ 8MHz
  //ADCSRA |= (1 << ADATE); // Enable auto-triggering
  ADCSRA |= (1 << ADEN);// Enable ADC
  ADCSRA |= (1 << ADIE);// Enable interrupts

  //ADMUX |= (1 << REFS0); // Set ADC reference to AVCC
  ADMUX |= (1 << MUX0) | (1 << MUX1) | (1 << MUX2);// Set ADC7 pin PA7
  ADCSRB |= (1 << ADLAR);// 8 bit resolution

  sei();// Enable Global Interrupts
  ADCSRA |= (1 << ADSC);// Start A2D Conversions


}

void loop() {
}

ISR(ADC_vect) {

  static uint8_t cnt = 0;
  static uint16_t sum = 0;

  cnt++;

  //Serial.println(ADCH);
  sum += ADCH;
  // make the average from 8 readings
  if (cnt >= 100) {
    //currentPotValue = constrain(map((sum / 100), 2, 250, 2, 200), 2, 200);
    currentPotValue = constrain(map((sum / 100), 2, 250, 2, 60), 2, 60);

    if ((currentPotValue < lastPotValue - 2 || currentPotValue > lastPotValue + 2) && currentPotValue != lastPotValue) {
      lastPotValue =  currentPotValue;
      SPEED_MANUAL = currentPotValue;
      set_step_delay(SPEED_MANUAL, is_forward);
      //Serial.println(SPEED_MANUAL);
    }
    cnt = 0;
    sum = 0;
  }

  if (manual_move_fwd || manual_move_bck) {
    //Serial.println(ADCH);
    ADCSRA |= (1 << ADSC); //Start A2D Conversions
  }
}

ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));

ISR(PCINT0_vect) {
  //if feeding steps impulses throw parallel port
  if (PINA & (1 << STEP_PIN)) {
    if (PINA & (1 << DIR_PIN)) {
      if (!is_min) nextStep(-1);
    }
    else {
      if (!is_max) nextStep(1);
    }
    sleep_motor = false;
  }

  //if stop min is achieved no more nextStep(-1)
  if (PINB & (1 << PCINT_STOP_MIN)) {
    is_min = true;
  }
  else {
    is_min = false;
  }
  //if stop_max is achieved no more nextStep(1)
  if (PINB & (1 << PCINT_STOP_MAX)) {
    is_max = true;
  }
  else {
    is_max = false;
  }

  //if manual move fwd button is pressed
  if ( PINA & (1 << MANUAL_FWD_PIN) ) {
    manual_move_fwd = true;
    set_step_delay(SPEED_MANUAL, true);
  }
  else {
    manual_move_fwd = false;
  }

  //if manual move back button is pressed
  if ( PINB & (1 << MANUAL_BCK_PIN) ) {
    manual_move_bck = true;
    set_step_delay(SPEED_MANUAL, false);
  }
  else {
    manual_move_bck = false;
  }

  if (!manual_move_bck && !manual_move_fwd) { //turn off the stepping LED when neither manual button is pressed
    PORTA &= ~(1 << STEP_PIN);
    PORTA &= ~(1 << DIR_PIN);
  }

  if ( /*!(PINA & (1 << STEP_PIN)) &&*/ !manual_move_fwd && !manual_move_bck && !sleep_motor ) {
    elapsed_sleep_motor = millis();
  }

}

ISR(TIM1_OVF_vect) {
  static boolean started_ADC = false;
  //if manual move is pressed, reset interrupts
  if (manual_move_fwd || manual_move_bck) {
    sleep_motor = false;
    elapsed_sleep_motor = millis();

    if (!started_ADC) {
      ADCSRA |= (1 << ADSC); // Start A2D Conversions only one time
      started_ADC = true;
    }

    //if enabled interrupt PCINT_STEP, disabled PCINT_STEP and PCINT_DIR because of the ISR(ISR_STEP_DIR)
    if (PCINT_MSK_0 & (1 << PCINT_STEP)) {
      PCINT_MSK_0 &= ~(1 << PCINT_STEP) & ~(1 << PCINT_DIR);
    }

    //if STEP_PIN is INPUT, set it to OUTPUT
    if (!(DDRA & (1 << STEP_PIN))) {
      DDRA |= 1 << STEP_PIN;
    }
    PORTA |= 1 << STEP_PIN;

    //if DIR_PIN is INPUT, set it to OUTPUT
    if (!(DDRA & (1 << DIR_PIN))) {
      DDRA |= 1 << DIR_PIN;
    }

    if (is_forward) {
      PORTA &= ~(1 << DIR_PIN);
      if (!is_max) {
        nextStep(1);
      }
    }
    else {
      PORTA |= 1 << DIR_PIN;
      if (!is_min) {
        nextStep(-1);
      }
    }
  }
  else {
    if (started_ADC) {
      started_ADC = false;
    }

    //reset the PCINT_STEP && PCINT_DIR interrupts and set STEP_PIN and DIR_PIN as INPUT again
    if (!(PCINT_MSK_0 & (1 << PCINT_STEP)) || !(PCINT_MSK_0 & (1 << PCINT_DIR))) {
      PCINT_MSK_0 |= (1 << PCINT_STEP) | (1 << PCINT_DIR);
    }
    if (DDRA & (1 << STEP_PIN)) {
      DDRA &= ~(1 << STEP_PIN);
    }
    if (DDRA & (1 << DIR_PIN)) {
      DDRA &= ~(1 << DIR_PIN);
    }

    //if are no more step pulses for over 1 second enter sleep mode
    if (/*!(PINA & (1 << STEP_PIN))  && */!sleep_motor && (millis() - elapsed_sleep_motor) > 1000) {
      stop_motor();
      sleep_motor = true;
    }
  }

  if (is_min && (PINA & (1 << STEP_PIN)) && (PINA & (1 << DIR_PIN)) && !sleep_motor) {
    stop_motor();
    sleep_motor = true;
  }

  if (is_max && (PINA & (1 << STEP_PIN)) && !(PINA & (1 << DIR_PIN)) && !sleep_motor) {
    stop_motor();
    sleep_motor = true;
  }

}

ISR(TIM1_COMPB_vect) {
  if (manual_move_fwd || manual_move_bck) {
    stop_motor();
    PORTA &= ~(1 << STEP_PIN);
  }
}

// between 0.016 and 1048 milliseconds (negative is in reverse direction)
void set_step_delay(float delayed, boolean set_dir) //milliseconds
{
  cli();
  is_forward = set_dir;
  delayed = abs(delayed);
  delay_long = (float)256 * (float)(1000 / delayed);

  OCR1A_temp = F_CPU / delay_long;
  OCR1A = (OCR1A_temp > 65535 ? 65535 : OCR1A_temp);

  
  /*if (delayed >= 15) {
    if (delayed >= 50) {
      OCR1B = (uint16_t) OCR1A / 10; // 1/10th duty cycle
    }
    else {
      OCR1B = (uint16_t) OCR1A / 3; // 1/10th duty cycle
    }
  }
  else {
    OCR1B = (uint16_t) OCR1A; // 100% duty cycle
  }*/
  
  
  if (delayed >= 5) {
    if (delayed >= 10) {
      OCR1B = (uint16_t) OCR1A / 10; // 1/10th duty cycle
    }
    else {
      OCR1B = (uint16_t) OCR1A / 3; // 1/10th duty cycle
    }
  }
  else {
    OCR1B = (uint16_t) OCR1A; // 100% duty cycle
  }
  
  sei();
}

void nextStep(uint8_t dir) { //dir FW = 1 RW = -1;
  current_seq = (current_seq + dir) % 4;
  full_step_seq(current_seq);
}

void full_step_seq(uint8_t seq) {
  switch (seq) {
    case 0:
      // 1010   AB
      PORTA &= ~(1 << STEPPER_PIN_A);
      PORTA |= 1 << STEPPER_PIN_NA;
      PORTA &= ~(1 << STEPPER_PIN_B);
      PORTA |= 1 << STEPPER_PIN_NB;
      break;
    case 1:
      // 0110   A'B
      PORTA &= ~(1 << STEPPER_PIN_A);
      PORTA |= 1 << STEPPER_PIN_NA;
      PORTA |= 1 << STEPPER_PIN_B;
      PORTA &= ~(1 << STEPPER_PIN_NB);
      break;
    case 2:
      // 0101   A'B'
      PORTA |= 1 << STEPPER_PIN_A;
      PORTA &= ~(1 << STEPPER_PIN_NA);
      PORTA |= 1 << STEPPER_PIN_B;
      PORTA &= ~(1 << STEPPER_PIN_NB);
      break;
    case 3:
      // 1001   AB'
      PORTA |= 1 << STEPPER_PIN_A;
      PORTA &= ~(1 << STEPPER_PIN_NA);
      PORTA &= ~(1 << STEPPER_PIN_B);
      PORTA |= 1 << STEPPER_PIN_NB;
      break;
  }
}

void stop_motor() {
  PORTA &= ~(1 << STEPPER_PIN_A);
  PORTA &= ~(1 << STEPPER_PIN_NA);
  PORTA &= ~(1 << STEPPER_PIN_B);
  PORTA &= ~(1 << STEPPER_PIN_NB);
}

