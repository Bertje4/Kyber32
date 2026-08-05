#include "Arduino.h"
#include <Esp.h>
#include <OneButton.h>
#include "configMenu.h"
#include "globalVariables.h"
#include "pinConfig.h"
#include "DFPlayer.h"
#include "esp_task_wdt.h"

#define MAX_SOUNDFONT 2
#define MAX_VOLUME 2
#define MAX_SWING_SENSITIVITY 16000

extern config_states config_state;
extern uint8_t soundFont;
extern uint8_t dfplayer_volume;
extern uint16_t swingSensitivity;
extern lightsaberColor MainColor;
extern lightsaberColor ClashColor;
extern lightsaberColor BlastColor;
extern uint32_t lightsaberColorHex[];

extern bool configChanged;
extern bool soundFontChanged;
extern bool configChangedUp;
extern bool configChangedDown;

extern DFPlayer audio;

extern SemaphoreHandle_t config_mutex;

ConfigMenu::ConfigMenu() {
}

void ConfigMenu::readConfig() {
  DEBUG_PRINTLN("readConfig: start");
  preferences.begin("Lightsaber", true);             // Lightsaber namespace, and false to be able to read/write

  DEBUG_PRINTLN("readConfig: preferences opened");

  soundFont = preferences.getUChar("SoundFont", 1);  // between 1 and 18
  DEBUG_PRINT("Readback Soundfont ");
  DEBUG_PRINTLN(soundFont);

  dfplayer_volume = preferences.getUChar("Volume", MAX_VOLUME);                             // between 0 and 30
  DEBUG_PRINT("Readback Volume ");
  DEBUG_PRINTLN(dfplayer_volume);

  swingSensitivity = preferences.getUShort("SwingSensitivity", SWING_SENSITIVITY_INITIAL);  // between 0 and 16000
  DEBUG_PRINT("Readback SwingSensitivity ");
  DEBUG_PRINTLN(swingSensitivity);

  MainColor = static_cast<lightsaberColor>(preferences.getUChar("MainColor", 0));           // Should be a enum number (0-20)
  DEBUG_PRINT("Readback MainColor ");
  DEBUG_PRINTLN(MainColor);

  ClashColor = static_cast<lightsaberColor>(preferences.getUChar("ClashColor", 3));         // Should be a enum number (0-20)
  DEBUG_PRINT("Readback ClashColor ");
  DEBUG_PRINTLN(ClashColor);

  BlastColor = static_cast<lightsaberColor>(preferences.getUChar("BlastColor", 13));        // Should be a enum number (0-20)
  DEBUG_PRINT("Readback BlastColor ");
  DEBUG_PRINTLN(BlastColor);

  lightsaberColorHex[18] = preferences.getUInt("UserColor1", 0xC0FF00);                     // Should be a uint32 hex color
  DEBUG_PRINTLN("Readback UserColor1 ok");

  lightsaberColorHex[19] = preferences.getUInt("UserColor2", 0x809BCE);                     // Should be a uint32 hex color
  DEBUG_PRINTLN("Readback UserColor2 ok");

  lightsaberColorHex[20] = preferences.getUInt("UserColor3", 0xF19953);                     // Should be a uint32 hex color
  DEBUG_PRINTLN("Readback UserColor3 ok");

  preferences.end();
  DEBUG_PRINTLN("readConfig: done");
}

void ConfigMenu::runConfigMenu(bool mainButtonPressed, bool secondaryButtonPressed) {
  switch (config_state) {
    case (config_soundfont):
      if (mainButtonPressed) {
        soundFontChanged = true;
        soundFont = (soundFont == MAX_SOUNDFONT) ? 1 : soundFont + 1;
        DEBUG_PRINT("increasing soundFont to ");
        DEBUG_PRINTLN(soundFont);
      }
      if (secondaryButtonPressed) {
        soundFontChanged = true;
        soundFont = (soundFont == 1) ? MAX_SOUNDFONT : soundFont - 1;
        DEBUG_PRINT("decreasing soundFont to ");
        DEBUG_PRINTLN(soundFont);
      }
      break;
    case (config_volume):
      if (mainButtonPressed) {
        configChangedUp = true;
        dfplayer_volume = (dfplayer_volume == MAX_VOLUME) ? MAX_VOLUME : dfplayer_volume + 1;
        DEBUG_PRINT("increasing volume to ");
        DEBUG_PRINTLN(dfplayer_volume);
      }
      if (secondaryButtonPressed) {
        configChangedDown = true;
        dfplayer_volume = (dfplayer_volume == 0) ? 0 : dfplayer_volume - 1;
        DEBUG_PRINT("decreasing volume to ");
        DEBUG_PRINTLN(dfplayer_volume);
      }
      break;
    case (config_swingsensitivity):
      if (mainButtonPressed) {
        configChangedUp = true;
        swingSensitivity = (swingSensitivity == MAX_SWING_SENSITIVITY) ? MAX_SWING_SENSITIVITY : swingSensitivity + 160;
        DEBUG_PRINT("increasing swingSensitivity to ");
        DEBUG_PRINTLN(swingSensitivity);
      }
      if (secondaryButtonPressed) {
        configChangedDown = true;
        swingSensitivity = (swingSensitivity == 0) ? 0 : swingSensitivity - 160;
        DEBUG_PRINT("decreasing swingSensitivity to ");
        DEBUG_PRINTLN(swingSensitivity);
      }
      break;
    case (config_maincolor):
      if (mainButtonPressed) {
        configChangedUp = true;
        MainColor = cycleColor(MainColor, true);
        DEBUG_PRINT("changed MainColor to ");
        DEBUG_PRINTLN(MainColor);
      }
      if (secondaryButtonPressed) {
        configChangedDown = true;
        MainColor = cycleColor(MainColor, false);
        DEBUG_PRINT("changed MainColor to ");
        DEBUG_PRINTLN(MainColor);
      }
      break;
    case (config_clashcolor):
      if (mainButtonPressed) {
        configChangedUp = true;
        ClashColor = cycleColor(ClashColor, true);
        DEBUG_PRINT("changed ClashColor to ");
        DEBUG_PRINTLN(ClashColor);
      }
      if (secondaryButtonPressed) {
        configChangedDown = true;
        ClashColor = cycleColor(ClashColor, false);
        DEBUG_PRINT("changed ClashColor to ");
        DEBUG_PRINTLN(ClashColor);
      }
      break;
    case (config_blastcolor):
      if (mainButtonPressed) {
        configChangedUp = true;
        BlastColor = cycleColor(BlastColor, true);
        DEBUG_PRINT("changed BlastColor to ");
        DEBUG_PRINTLN(BlastColor);
      }
      if (secondaryButtonPressed) {
        configChangedDown = true;
        BlastColor = cycleColor(BlastColor, false);
        DEBUG_PRINT("changed BlastColor to ");
        DEBUG_PRINTLN(BlastColor);
      }
      break;
    case (config_batteryLevel):
      break;
    default:
      // config_idle:
      break;
  }
}

// Called from Buttons task — just sets a flag, no flash I/O
void ConfigMenu::saveConfigMenu() {
  DEBUG_PRINTLN("Config save requested (deferred)");
  pendingSave = true;
}

// Called from DFPlayer task where stack is large enough
void ConfigMenu::doSave() {
  DEBUG_PRINT(" ***** [configMenu]: Saving new Config: ");
  DEBUG_PRINT(soundFont);
  DEBUG_PRINTLN(" ***** ");

  preferences.begin("Lightsaber", false);

  preferences.putUChar("SoundFont",         soundFont);

  preferences.putUChar("Volume",            dfplayer_volume);

  preferences.putUShort("SwingSensitivity", swingSensitivity);

  preferences.putUChar("MainColor",         static_cast<uint8_t>(MainColor));

  preferences.putUChar("ClashColor",        static_cast<uint8_t>(ClashColor));

  preferences.putUChar("BlastColor",        static_cast<uint8_t>(BlastColor));

  preferences.end();
  DEBUG_PRINTLN("[configMenu]: Preferences.end");

  pendingVolumeChange = true;
  pendingSave = false;
  DEBUG_PRINTLN("[configMenu]: Activated volumelevel flag");

  DEBUG_PRINTLN(" ***** [configMenu]: doSave has ended *****");
}

lightsaberColor ConfigMenu::cycleColor(lightsaberColor color, bool increase) {
  uint8_t index = static_cast<uint8_t>(color);
  if (increase) {
    index = (index == (NumColors - 1)) ? 0 : index + 1;
  } else {
    index = (index == 0) ? (NumColors - 1) : index - 1;
  }
  return static_cast<lightsaberColor>(index);
}

void ConfigMenu::nextConfigMenu() {
  xSemaphoreTake(config_mutex, portMAX_DELAY);
  configChanged = true;
  uint8_t state = static_cast<uint8_t>(config_state);
  state = (state == (config_lastMember - 1)) ? 1 : state + 1;
  config_state = static_cast<config_states>(state);
  DEBUG_PRINT("set config_state ");
  DEBUG_PRINTLN(config_state);
  xSemaphoreGive(config_mutex);
}

void ConfigMenu::prevConfigMenu() {
  xSemaphoreTake(config_mutex, portMAX_DELAY);
  configChanged = true;
  uint8_t state = static_cast<uint8_t>(config_state);
  state = (state == 1) ? (config_lastMember - 1) : state - 1;
  config_state = static_cast<config_states>(state);
  DEBUG_PRINT("set config_state ");
  DEBUG_PRINTLN(config_state);
  xSemaphoreGive(config_mutex);
}