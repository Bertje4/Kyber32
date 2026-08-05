#ifndef LEDS_H
#define LEDS_H
#include <FastLED.h>
#include "globalVariables.h"
#include "pinConfig.h"

// Total number of addressable LEDs on the data line:
//   parallel mode  → NUM_LEDS  (both strips share one pin, act identically)
//   series mode    → 2*NUM_LEDS (one continuous U-shaped strip)
#ifdef STRIPS_IN_SERIES
  #define TOTAL_LEDS (2 * NUM_LEDS)
#else
  #define TOTAL_LEDS NUM_LEDS
#endif

class Blade {
private:
  // Code for LED creating objects
  CRGB leds_output_array[TOTAL_LEDS];
  CRGB crystal_output_array[NUM_LEDS_CRYSTAL > 0 ? NUM_LEDS_CRYSTAL : 1]; // guard against zero-length array

  // Code for task creation and running
  static void runTask(void* pvParameters);
  void LEDCode();

  // Code for helper functions in tasks
  void initLEDS();
  void setLedsWithFlicker(lightsaberColor color);
  void setSolidColor(CRGB color);
  void addClashToLeds();
  void addBlasterToLeds();
  void setLedsToLockup();
  void setLedsToTipmelt();
  void setColorOrRainbow(lightsaberColor color);
  void fillRainbowLEDs(CRGB* leds, int count, uint16_t baseHue);
  void fillFlickerLEDs(CRGB* leds, int count, CRGB color, uint16_t& seed, uint16_t speed);

  uint16_t colorNoiseSeed = 0;
  uint16_t colorNoiseSpeed = 2;
  unsigned long clashStartTime = 0;
  const int pulseInterval = 20;  // ms between pulses

public:
  Blade();
  void startTask();
};

extern bool leds_ready;

#endif
