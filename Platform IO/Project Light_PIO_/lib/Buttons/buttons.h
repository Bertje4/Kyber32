#ifndef BUTTON_H
#define BUTTON_H
#include <OneButton.h>

#define CLICK 200
#define LONG_PRESS 400

// Blink periods (ms) for single-LED buttons
#define SINGLE_LED_BLINK_SLOW_MS  500   // config menu
#define SINGLE_LED_BLINK_FAST_MS  150   // web config

enum button_types {
  button_single,
  button_double_main
#ifndef SINGLE_BUTTON_MODE
  , button_double_secondary
#endif
};

// Modes used by single-LED (non-RGB) buttons
enum SingleLEDMode {
  single_led_off,        // idle        — LED off
  single_led_on,         // saber on    — LED solid
  single_led_slow_blink, // config menu — slow blink
  single_led_fast_blink  // web config  — fast blink
};

class Buttons {
private:
  button_types current_button_type;
  OneButton button;

  static void runTask(void* pvParameters);
  void ButtonsCode();

  void initButton();
  void initRGBPins(uint8_t rPin, uint8_t gPin, uint8_t bPin);
  void initSingleLEDPin(uint8_t ledPin);
  void setLEDColorForButton(button_types btn, bool r, bool g, bool b);
  void setLEDColor(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool r, bool g, bool b);
  void tickSingleLED(uint8_t ledPin, SingleLEDMode mode);

  // Per-instance blink state (one Buttons object per physical button)
  bool           singleLEDState    = false;
  unsigned long  singleLEDLastFlip = 0;

  static void main_button_click();
  static void main_button_doubleclick();
  static void main_button_longPressStart();
  static void main_button_longPress();
  static void main_button_longPressStop();

#ifndef SINGLE_BUTTON_MODE
  static void secondary_button_click();
  static void secondary_button_doubleclick();
  static void secondary_button_longPressStart();
  static void secondary_button_longPress();
  static void secondary_button_longPressStop();
#endif

public:
  Buttons(button_types button_type);
  void startTask();
};

extern bool buttons_ready;

#endif