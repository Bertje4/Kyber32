#ifndef SABERWEB_H
#define SABERWEB_H

#include <Esp.h>
#include <Preferences.h>
#include <LittleFS.h>
#include "globalVariables.h"
#include "pinConfig.h"
#include "ESPAsyncWebServer.h"

class SaberWeb {
private:
  Preferences preferences;

  static void runTask(void* pvParameters);
  void WEBCode();

  void initSaberWeb();
  void runSaberWeb();
  void saveSaberWeb(AsyncWebServerRequest* request);

  static SaberWeb* instance;
  TaskHandle_t saberWebTaskHandle = NULL;

public:
  SaberWeb();
  void startTask();
  void stopTask();
};

#endif
