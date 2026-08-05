#include "esp32-hal-gpio.h"
#include <Esp.h>
#include <OneButton.h>
#include "buttons.h"
#include "pinConfig.h"
#include "globalVariables.h"
#include "configMenu.h"
#include "SaberWeb.h"

// SaberWeb must NOT be constructed as a global — its constructor runs before
// the ESP32 NVS/hardware is ready, which corrupts Preferences in readConfig().
// Use a pointer and construct it on first use instead.
static SaberWeb* saberwebConfig = nullptr;

static SaberWeb& getSaberWeb() {
  if (saberwebConfig == nullptr) {
    saberwebConfig = new SaberWeb();
  }
  return *saberwebConfig;
}

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
  DEBUG_PRINTLN("Buttons::startTask()");
  #ifdef DUAL_CORE
    xTaskCreatePinnedToCore(
      runTask,
      "ButtonsTask",
      BUTTONS_TASK_STACK_SIZE,
      this,
      BUTTONS_TASK_PRIORITY,
      NULL,
      1
    );
  #else
   xTaskCreate(
      runTask,
      "ButtonsTask",
      BUTTONS_TASK_STACK_SIZE,
      this,
      BUTTONS_TASK_PRIORITY,
      NULL
    );
  #endif
}

void Buttons::runTask(void* pvParameters) {
  DEBUG_PRINT("ButtonTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
  Buttons* instance = static_cast<Buttons*>(pvParameters);
  instance->ButtonsCode();
}

void Buttons::initButton() {
  DEBUG_PRINTLN("Buttons::initButton()");
  if (current_button_type == button_double_main) {
    DEBUG_PRINTLN("initButton: setting up main button");
    button.setup(MAIN_BUTTON, MAIN_BUTTON_PULL_STATE, MAIN_BUTTON_ACTIVE_STATE);
    button.setClickMs(CLICK);
    button.setPressMs(LONG_PRESS);
    button.attachClick(main_button_click);
    button.attachDoubleClick(main_button_doubleclick);
    button.attachLongPressStart(main_button_longPressStart);
    button.attachLongPressStop(main_button_longPressStop);
    button.attachDuringLongPress(main_button_longPress);

#ifdef MAIN_BUTTON_RGB
    initRGBPins(MAIN_RED, MAIN_GREEN, MAIN_BLUE);
#else
    initSingleLEDPin(MAIN_LED);
#endif

#ifndef SINGLE_BUTTON_MODE
  } else if (current_button_type == button_double_secondary) {
    button.setup(SECOND_BUTTON, SECOND_BUTTON_PULL_STATE, SECOND_BUTTON_ACTIVE_STATE);
    button.setClickMs(CLICK);
    button.setPressMs(LONG_PRESS);
    button.attachClick(secondary_button_click);
    button.attachDoubleClick(secondary_button_doubleclick);
    button.attachLongPressStart(secondary_button_longPressStart);
    button.attachLongPressStop(secondary_button_longPressStop);
    button.attachDuringLongPress(secondary_button_longPress);

#ifdef SECOND_BUTTON_RGB
    initRGBPins(SECOND_RED, SECOND_GREEN, SECOND_BLUE);
#else
    initSingleLEDPin(SECOND_LED);
#endif
#endif
  }
}

// define a wrapper to easily replace the DigitalWrite they use here!
void LED_ON(uint8_t ledPin) {
  #ifdef BUTTON_LEDS_WIRED_TO_3V3 
    digitalWrite(ledPin, LOW);
  #else
    digitalWrite(ledPin, HIGH);
  #endif
}

void LED_OFF(uint8_t ledPin) {
  #ifdef BUTTON_LEDS_WIRED_TO_3V3 
    digitalWrite(ledPin, HIGH);
  #else
    digitalWrite(ledPin, LOW);
  #endif
}

void Buttons::initRGBPins(uint8_t rPin, uint8_t gPin, uint8_t bPin) {
  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
  LED_OFF(rPin);
  LED_OFF(gPin);
  LED_OFF(bPin);
}

void Buttons::initSingleLEDPin(uint8_t ledPin) {
  pinMode(ledPin, OUTPUT);
  LED_OFF(ledPin);  // off at start — respects BUTTON_LEDS_WIRED_TO_3V3
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
    // ── Charge-safe mode: disable button input and LEDs ──────────────────────
    if (global_state == lightsaber_charging) {
      // Do NOT call button.tick() — we don't want any button actions while charging.
      // Turn all button LEDs off.
      setLEDColorForButton(current_button_type, LOW, LOW, LOW);
      vTaskDelay(pdMS_TO_TICKS(200));
      xLastWakeTime = xTaskGetTickCount();
      continue;
    }

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
  // Derive the single-LED mode from the same r/g/b signals the RGB path uses:
  //   blue only  (0,0,1) → saber on    → solid
  //   green only (0,1,0) → config/web  → need to distinguish further
  //   red only   (1,0,0) → idle        → off
  // Web-config and config both arrive as green here, so we inspect global state
  // directly to tell them apart.
  SingleLEDMode mode;
  if (!r && !g && !b) {
    mode = single_led_off;
  } else if (!r && !g && b) {
    mode = single_led_on;
  } else if (!r && g && !b) {
    // green = config menu OR web-config
    if (lightsaber_on_state == lightsaber_on_web_config) {
      mode = single_led_fast_blink;
    } else {
      mode = single_led_slow_blink;
    }
  } else {
    // red (idle) → off; any other unexpected combo → off
    mode = single_led_off;
  }

  if (btn == button_double_main) {
#ifdef MAIN_BUTTON_RGB
    setLEDColor(MAIN_RED, MAIN_GREEN, MAIN_BLUE, r, g, b);
#else
    tickSingleLED(MAIN_LED, mode);
#endif
#ifndef SINGLE_BUTTON_MODE
  } else if (btn == button_double_secondary) {
#ifdef SECOND_BUTTON_RGB
    setLEDColor(SECOND_RED, SECOND_GREEN, SECOND_BLUE, r, g, b);
#else
    tickSingleLED(SECOND_LED, mode);
#endif
#endif
  }
}

void Buttons::setLEDColor(uint8_t rPin, uint8_t gPin, uint8_t bPin, bool r, bool g, bool b) {
  r ? LED_ON(rPin) : LED_OFF(rPin);
  g ? LED_ON(gPin) : LED_OFF(gPin);
  b ? LED_ON(bPin) : LED_OFF(bPin);
}

// Called every task tick. Drives the single LED according to the requested mode,
// using millis()-based timing so no extra timer or task is needed.
void Buttons::tickSingleLED(uint8_t ledPin, SingleLEDMode mode) {
  switch (mode) {
    case single_led_off:
      singleLEDState = false;
      LED_OFF(ledPin);
      break;

    case single_led_on:
      singleLEDState = true;
      LED_ON(ledPin);
      break;

    case single_led_slow_blink:
    case single_led_fast_blink: {
      unsigned long period = (mode == single_led_slow_blink)
                             ? SINGLE_LED_BLINK_SLOW_MS
                             : SINGLE_LED_BLINK_FAST_MS;
      unsigned long now = millis();
      if (now - singleLEDLastFlip >= period) {
        singleLEDLastFlip = now;
        singleLEDState    = !singleLEDState;
        singleLEDState ? LED_ON(ledPin) : LED_OFF(ledPin);
      }
      break;
    }
  }
}

// ----- Main Button Callbacks -----

void Buttons::main_button_click() {
  if (global_state == lightsaber_charging) return;  // ignore all input while charging
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
  if (global_state == lightsaber_charging) return;
  DEBUG_PRINTLN("Main Button doubleclick.");
#ifdef SINGLE_BUTTON_MODE
  if (global_state == lightsaber_idle) {
    if (lightsaber_on_state == lightsaber_on_idle) {
      DEBUG_PRINTLN("Saber web task started from Button.");
      getSaberWeb().startTask();
      lightsaber_on_state = lightsaber_on_web_config;
    } else if (lightsaber_on_state == lightsaber_on_web_config) {
      DEBUG_PRINTLN("Saber web task stopped from Button.");
      getSaberWeb().stopTask();
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
      getSaberWeb().startTask();
      lightsaber_on_state = lightsaber_on_web_config;
    } else if (lightsaber_on_state == lightsaber_on_web_config) {
      DEBUG_PRINTLN("Saber web task stopped from Button.");
      getSaberWeb().stopTask();
      lightsaber_on_state = lightsaber_on_idle;
    }
  }
#endif
}

void Buttons::main_button_longPressStart() {
  if (global_state == lightsaber_charging) return;
  DEBUG_PRINTLN("Main Button longPress start");

#ifdef SINGLE_BUTTON_MODE
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
  if (global_state == lightsaber_charging) return;
  DEBUG_PRINTLN("Main Button longPress...");
#ifdef SINGLE_BUTTON_MODE
  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_hum) {
      if (millis() - longPressStartTime > LONG_PRESS_LOCKUP_MS) { // LONG_PRESS_LOCKUP_MS = 4000
        lightsaber_on_state = lightsaber_on_bladelockup;
        DEBUG_PRINTLN("[longPress]: setting lightsaber_on_bladelockup");

      } else {
        lightsaber_on_state = lightsaber_on_hum;
        DEBUG_PRINTLN("[longPress]: setting lightsaber_on_hum");
      }
    }
    DEBUG_PRINT("[longPress]: check: lightsaber on state:");
    DEBUG_PRINTLN(lightsaber_on_state);
  }
#endif
}

void Buttons::main_button_longPressStop() {
  DEBUG_PRINTLN("Main Button longPress stop");
#ifdef SINGLE_BUTTON_MODE
  DEBUG_PRINT("[Button]: global_state = ");
  DEBUG_PRINT(globalStateToString(global_state));

  DEBUG_PRINT(" | lightsaber_on_state = ");
  DEBUG_PRINTLN(onStateToString(lightsaber_on_state));

  if (global_state == lightsaber_on) {
    if (lightsaber_on_state == lightsaber_on_bladelockup) {
      lightsaber_on_state = lightsaber_on_hum;
      DEBUG_PRINTLN("HUMMING!");

    } else if (lightsaber_on_state == lightsaber_on_hum) {
      lightsaber_on_state = lightsaber_on_retraction;
      DEBUG_PRINTLN("RETRACTING");
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