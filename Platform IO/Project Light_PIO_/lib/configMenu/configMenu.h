#ifndef CONFIGMENU_H
#define CONFIGMENU_H

#include "Arduino.h"
#include <Preferences.h>
#include <Esp.h>

#include "globalVariables.h"
#include "pinConfig.h"



class ConfigMenu {
private:
  // Code for button objects
  Preferences preferences;
  lightsaberColor cycleColor(lightsaberColor color, bool increase);

public:
  // Code for button objects
  ConfigMenu();
  void readConfig();
  void nextConfigMenu();
  void prevConfigMenu();
  void saveConfigMenu();
  void runConfigMenu(bool mainButtonPressed, bool secondaryButtonPressed);


  // Code for helper functions in tasks
  // static volatile bool pendingSave;
  void doSave();   // the actual NVS write — call from a task with enough stack
};


#endif