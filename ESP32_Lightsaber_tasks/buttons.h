#ifndef BUTTON_H
#define BUTTON_H
#include <OneButton.h>

#define CLICK 200
#define LONG_PRESS 400

enum button_types {
  button_single,
  button_double_main
#ifndef SINGLE_BUTTON_MODE
  , button_double_secondary
#endif
};

class Buttons {
private:
  button_types current_button_type;
  OneButton button;

  static void runTask(void* pvParameters);
  void ButtonsCode();

  void initButton();
  void initRGBPins(uint8_t rPin, uint8_t gPin, uint8_t bPin);
  void setLEDColorForButton(button_types btn, bool r, bool g, bool b);
  void setLEDColor(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool r, bool g, bool b);

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