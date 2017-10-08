#include "FastLED.h"

#define NUM_LEDS 8
#define DATA_PIN 6
CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS);
}

// This function runs over and over, and is where you do the magic to light
// your leds.
void loop() {
    FastLED.setBrightness(10);
    leds[0] = Candle;
    leds[1] = CRGB::MediumOrchid;
    leds[2] = CRGB::Green;
    leds[3] = CRGB::Olive;
    leds[4] = CRGB::Maroon;
    leds[5] = CRGB::DodgerBlue;
    leds[6] = CRGB::SandyBrown;
    leds[7] = CRGB::DarkGoldenrod;
    FastLED.show();
}
