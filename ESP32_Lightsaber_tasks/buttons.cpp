#include <Esp.h>
#include <OneButton.h>
#include "buttons.h"
#include "pinConfig.h"
#include "globalVariables.h"
#include "configMenu.h"
#include "SaberWeb.h"

SaberWeb saberwebConfig = SaberWeb();

extern global_states global_state;
extern lightsaber_on_states lightsaber_on_state;
extern config_states config_state;
extern bool configStart;
extern ConfigMenu menu;

extern uint8_t effectLeds;
extern uint8_t effectLedsLength;

// Add near the top of buttons.cpp, after the extern declarations:
#ifdef SINGLE_BUTTON_MODE
  static unsigned long longPressStartTime = 0;
#endif

bool buttons_ready = false;
bool blaster_enabled = false;
bool lockup_enabled = false;

Buttons::Buttons(button_types button_type)
  : current_button_type(button_type) {
}

void Buttons::startTask() {
  xTaskCreatePinnedToCore(
    runTask,
    "ButtonsTask",
    BUTTONS_TASK_STACK_SIZE,
    this,
    BUTTONS_TASK_PRIORITY,
    NULL,
    1);
}

void Buttons::runTask(void* pvParameters) {
  DEBUG_PRINT("ButtonTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
  Buttons* instance = static_cast<Buttons*>(pvParameters);
  instance->ButtonsCode();
}

void Buttons::initButton() {
  if (current_button_type == button_double_main) {
    button.setup(MAIN_BUTTON, INPUT_PULLUP, true);
    button.setClickMs(CLICK);
    button.setPressMs(LONG_PRESS);
    button.attachClick(main_button_click);
    button.attachDoubleClick(main_button_doubleclick);
    button.attachLongPressStart(main_button_longPressStart);
    button.attachLongPressStop(main_button_longPressStop);
    button.attachDuringLongPress(main_button_longPress);

    initRGBPins(MAIN_RED, MAIN_GREEN, MAIN_BLUE);

#ifndef SINGLE_BUTTON_MODE
  } else if (current_button_type == button_double_secondary) {
    button.setup(SECOND_BUTTON, INPUT_PULLUP, true);
    button.setClickMs(CLICK);
    button.setPressMs(LONG_PRESS);
    button.attachClick(secondary_button_click);
    button.attachDoubleClick(secondary_button_doubleclick);
    button.attachLongPressStart(secondary_button_longPressStart);
    button.attachLongPressStop(secondary_button_longPressStop);
    button.attachDuringLongPress(secondary_button_longPress);

    initRGBPins(SECOND_RED, SECOND_GREEN, SECOND_BLUE);
#endif
  }
}

void Buttons::initRGBPins(uint8_t rPin, uint8_t gPin, uint8_t bPin) {
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  digitalWrite(rPin, HIGH);
  digitalWrite(gPin, HIGH);
  digitalWrite(bPin, HIGH);
}

void Buttons::ButtonsCode() {
  DEBUG_PRINT("Button running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS((1000 / BUTTONS_HZ));

  initButton();
  buttons_ready = true;

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    button.tick();

    if (global_state == lightsaber_on) {
      vTaskPrioritySet(NULL, BUTTONS_TASK_PRIORITY);
      setLEDColorForButton(current_button_type, LOW, LOW, HIGH);
    } else if (global_state == lightsaber_config) {
      vTaskPrioritySet(NULL, BUTTONS_TASK_PRIORITY + 1);
      setLEDColorForButton(current_button_type, LOW, HIGH, LOW);
    } else {
      vTaskPrioritySet(NULL, BUTTONS_TASK_PRIORITY);
      if (lightsaber_on_state == lightsaber_on_web_config) {
        setLEDColorForButton(current_button_type, LOW, HIGH, LOW);
      } else {
        setLEDColorForButton(current_button_type, HIGH, LOW, LOW);
      }
    }
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void Buttons::setLEDColorForButton(button_types btn, bool r, bool g, bool b) {
  if (btn == button_double_main) {
    setLEDColor(MAIN_RED, MAIN_GREEN, MAIN_BLUE, r, g, b);
#ifndef SINGLE_BUTTON_MODE
  } else if (btn == button_double_secondary) {
    setLEDColor(SECOND_RED, SECOND_GREEN, SECOND_BLUE, r, g, b);
#endif
  }
}

void Buttons::setLEDColor(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool r, bool g, bool b) {
  digitalWrite(rPin, !r);
  digitalWrite(gPin, !g);
  digitalWrite(bPin, !b);
}

// ----- Main Button Callbacks -----

void Buttons::main_button_click() {
  DEBUG_PRINTLN("Main Button click.");

#ifdef SINGLE_BUTTON_MODE
  static uint8_t clickCount = 0;
  static unsigned long lastClickTime = 0;

  unsigned long now = millis();
  if (now - lastClickTime < 400) {
    clickCount++;
  } else {
    clickCount = 1;
  }
  lastClickTime = now;

  if (clickCount == 3) {
    clickCount = 0;
    if (global_state == lightsaber_on && lightsaber_on_state == lightsaber_on_hum) {
      effectLeds = random(effectLedsLength, NUM_LEDS);
      lightsaber_on_state = lightsaber_on_tipmelt;
      vTaskDelay(TIPMELT_FX_DURATION);
      lightsaber_on_state = lightsaber_on_hum;
    }
    return;
  }
#endif

  if (global_state == lightsaber_idle) {
    if (lightsaber_on_state == lightsaber_on_idle) {
      global_state = lightsaber_on;
      lightsaber_on_state = lightsaber_on_ignition;
    }
  } else if (global_state == lightsaber_config) {
    menu.runConfigMenu(true, false);
  } else if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
#ifdef SINGLE_BUTTON_MODE
      blaster_enabled = true;
#endif
    }
  }
}

void Buttons::main_button_doubleclick() {
  DEBUG_PRINTLN("Main Button doubleclick.");
#ifdef SINGLE_BUTTON_MODE
  if (global_state == lightsaber_idle) {
    if (lightsaber_on_state == lightsaber_on_idle) {
      DEBUG_PRINTLN("Saber web task started from Button.");
      saberwebConfig.startTask();
      lightsaber_on_state = lightsaber_on_web_config;
    } else if (lightsaber_on_state == lightsaber_on_web_config) {
      DEBUG_PRINTLN("Saber web task stopped from Button.");
      saberwebConfig.stopTask();
      lightsaber_on_state = lightsaber_on_idle;
    }
  } else if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      lockup_enabled = true;
    }
  } else if (global_state == lightsaber_config) {
    menu.runConfigMenu(false, true);
  }
#else
  // Original two-button behaviour
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      effectLeds = random(effectLedsLength, NUM_LEDS);
      lightsaber_on_state = lightsaber_on_tipmelt;
      vTaskDelay(TIPMELT_FX_DURATION);
      lightsaber_on_state = lightsaber_on_hum;
    }
  } else if (global_state == lightsaber_idle) {
    if (lightsaber_on_state == lightsaber_on_idle) {
      DEBUG_PRINTLN("Saber web task started from Button.");
      saberwebConfig.startTask();
      lightsaber_on_state = lightsaber_on_web_config;
    } else if (lightsaber_on_state == lightsaber_on_web_config) {
      DEBUG_PRINTLN("Saber web task stopped from Button.");
      saberwebConfig.stopTask();
      lightsaber_on_state = lightsaber_on_idle;
    }
  }
#endif
}

void Buttons::main_button_longPressStart() {
  DEBUG_PRINTLN("Main Button longPress start");

#ifdef SINGLE_BUTTON_MODE
  static unsigned long longPressStartTime = 0;  // moved here as static local
  longPressStartTime = millis();

  if (global_state == lightsaber_idle) {
    global_state = lightsaber_config;
    config_state = config_idle;
    configStart = true;
  } else if (global_state == lightsaber_config) {
    menu.nextConfigMenu();
  }
#else
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      lightsaber_on_state = lightsaber_on_retraction;
    }
  } else if (global_state == lightsaber_config) {
    menu.nextConfigMenu();
  }
#endif
}

void Buttons::main_button_longPress() {
  DEBUG_PRINTLN("Main Button longPress...");
#ifdef SINGLE_BUTTON_MODE
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      if (millis() - longPressStartTime > LONG_PRESS_LOCKUP_MS) {
        lightsaber_on_state = lightsaber_on_bladelockup;
      }
    }
  }
#endif
}

void Buttons::main_button_longPressStop() {
  DEBUG_PRINTLN("Main Button longPress stop");
#ifdef SINGLE_BUTTON_MODE
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_bladelockup) {
      lightsaber_on_state = lightsaber_on_hum;
    } else if (lightsaber_on_state == lightsaber_on_hum) {
      lightsaber_on_state = lightsaber_on_retraction;
    }
  } else if (global_state == lightsaber_config) {
    menu.saveConfigMenu();
    global_state = lightsaber_idle;
    lightsaber_on_state = lightsaber_on_boot;
    config_state = config_idle;
    configStart = true;
  }
#endif
}

// ----- Secondary Button Callbacks (two-button mode only) -----

#ifndef SINGLE_BUTTON_MODE
void Buttons::secondary_button_click() {
  DEBUG_PRINTLN("Secondary Button click.");
  if (global_state == lightsaber_config) {
    menu.runConfigMenu(false, true);
  } else if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      effectLeds = random(effectLedsLength, NUM_LEDS);
      lightsaber_on_state = lightsaber_on_blasterdeflect;
      vTaskDelay(BLASTER_FX_DURATION);
      lightsaber_on_state = lightsaber_on_hum;
    }
  }
}

void Buttons::secondary_button_doubleclick() {
  DEBUG_PRINTLN("Secondary Button doubleclick.");
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      lockup_enabled = true;
    }
  }
}

void Buttons::secondary_button_longPressStart() {
  if (global_state == lightsaber_idle) {
    global_state = lightsaber_config;
    config_state = config_idle;
    configStart = true;
  } else if (global_state == lightsaber_config) {
    menu.saveConfigMenu();
    global_state = lightsaber_idle;
    lightsaber_on_state = lightsaber_on_boot;
    config_state = config_idle;
    configStart = true;
  } else if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      lightsaber_on_state = lightsaber_on_bladelockup;
    }
  }
  DEBUG_PRINTLN("Secondary Button longPress start");
}

void Buttons::secondary_button_longPress() {
  DEBUG_PRINTLN("Secondary Button longPress...");
}

void Buttons::secondary_button_longPressStop() {
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_bladelockup) {
      lightsaber_on_state = lightsaber_on_hum;
    }
  }
  DEBUG_PRINTLN("Secondary Button longPress stop");
}
#endif