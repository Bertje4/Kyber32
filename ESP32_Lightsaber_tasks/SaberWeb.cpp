#include <Esp.h>
#include <OneButton.h>
#include "SaberWeb.h"
#include "globalVariables.h"
#include "pinConfig.h"
#include "DFPlayer.h"

#include <DNSServer.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include <LittleFS.h>

const char* ssid = "SaberWeb";
const char* password = "lightsaber123";

AsyncWebServer server(80);
SaberWeb* SaberWeb::instance = nullptr;
DNSServer dnsServer;

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

SaberWeb::SaberWeb() {
  saberWebTaskHandle = NULL;
  SaberWeb::instance = this;
}

void SaberWeb::startTask() {
  DEBUG_PRINTLN("Saber web task started.");
  xTaskCreatePinnedToCore(
    runTask,
    "WEBTask",
    8192,
    this,
    1,
    &saberWebTaskHandle,
    0);
}

void SaberWeb::stopTask() {
  if (saberWebTaskHandle != NULL) {
    vTaskDelete(saberWebTaskHandle);
    saberWebTaskHandle = NULL;
    // Shut down WiFi and server cleanly
    server.end();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    DEBUG_PRINTLN("Saber web task stopped.");
  }
}

void SaberWeb::runTask(void* pvParameters) {
  SaberWeb* instance = static_cast<SaberWeb*>(pvParameters);
  instance->WEBCode();
}

void SaberWeb::WEBCode() {
  DEBUG_PRINT("WEBTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / 60);

  initSaberWeb();

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    runSaberWeb();
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void SaberWeb::initSaberWeb() {
  // Mount LittleFS
  if (!LittleFS.begin(true)) {
    DEBUG_PRINTLN("LittleFS mount failed!");
    return;
  }
  DEBUG_PRINTLN("LittleFS mounted.");

  // Check the HTML file is present
  if (!LittleFS.exists("/SaberWeb.html")) {
    DEBUG_PRINTLN("WARNING: /SaberWeb.html not found on LittleFS!");
    DEBUG_PRINTLN("Upload the data/ folder using 'Upload Filesystem Image'.");
  }

  // Start Access Point
  if (WiFi.softAP(ssid, password)) {
    DEBUG_PRINTLN("Access Point Started");
    DEBUG_PRINT("IP Address: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
  }

  // Serve index from LittleFS
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/SaberWeb.html", "text/html");
  });

  // Captive portal: redirect all unknown URLs to index
  server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(LittleFS, "/SaberWeb.html", "text/html");
  });

  // Handle form submission
  server.on("/submit", HTTP_GET, [](AsyncWebServerRequest* request) {
    instance->saveSaberWeb(request);
    // Send a styled confirmation page
    request->send(200, "text/html",
      "<!DOCTYPE html><html><head>"
      "<meta charset='UTF-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<meta http-equiv='refresh' content='2;url=/'>"
      "<style>"
      "body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;"
      "background:#050d1a;font-family:sans-serif;color:#00BFFF;}"
      "h2{font-size:1.4rem;letter-spacing:0.1em;text-align:center;}"
      "p{color:#aaa;text-align:center;font-size:0.9rem;margin-top:8px;}"
      "</style></head><body>"
      "<div><h2>&#10003; Settings Saved</h2><p>Returning to config...</p></div>"
      "</body></html>"
    );
  });

  // Start DNS captive portal (redirects all domains to ESP IP)
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.setTTL(300);
  dnsServer.start(53, "*", WiFi.softAPIP());

  server.begin();
}

void SaberWeb::runSaberWeb() {
  dnsServer.processNextRequest();
}

void SaberWeb::saveSaberWeb(AsyncWebServerRequest* request) {
  String str_soundFont         = request->getParam("SoundFont")->value();
  String str_volume            = request->getParam("Volume")->value();
  String str_swingSensitivity  = request->getParam("SwingSensitivity")->value();
  String str_mainColor         = request->getParam("mainColor")->value();
  String str_blastColor        = request->getParam("blastColor")->value();
  String str_clashColor        = request->getParam("clashColor")->value();
  String str_customColor1      = request->getParam("UserColor1")->value().substring(1);
  String str_customColor1Hex   = request->getParam("UserColor1Hex")->value();
  String str_customColor2      = request->getParam("UserColor2")->value().substring(1);
  String str_customColor2Hex   = request->getParam("UserColor2Hex")->value();
  String str_customColor3      = request->getParam("UserColor3")->value().substring(1);
  String str_customColor3Hex   = request->getParam("UserColor3Hex")->value();

  uint8_t  web_soundFont        = str_soundFont.toInt();
  uint8_t  web_volume           = str_volume.toInt();
  uint16_t web_swingSensitivity = str_swingSensitivity.toInt();
  uint8_t  web_mainColor        = str_mainColor.toInt();
  uint8_t  web_blastColor       = str_blastColor.toInt();
  uint8_t  web_clashColor       = str_clashColor.toInt();
  uint32_t web_customColor1     = (uint32_t)strtol(str_customColor1.c_str(), NULL, 16);
  uint32_t web_customColor1Hex  = (uint32_t)strtol(str_customColor1Hex.c_str(), NULL, 16);
  uint32_t web_customColor2     = (uint32_t)strtol(str_customColor2.c_str(), NULL, 16);
  uint32_t web_customColor2Hex  = (uint32_t)strtol(str_customColor2Hex.c_str(), NULL, 16);
  uint32_t web_customColor3     = (uint32_t)strtol(str_customColor3.c_str(), NULL, 16);
  uint32_t web_customColor3Hex  = (uint32_t)strtol(str_customColor3Hex.c_str(), NULL, 16);

  // Hex text field overrides color picker if filled in
  if (str_customColor1Hex.length() > 0) web_customColor1 = web_customColor1Hex;
  if (str_customColor2Hex.length() > 0) web_customColor2 = web_customColor2Hex;
  if (str_customColor3Hex.length() > 0) web_customColor3 = web_customColor3Hex;

  DEBUG_PRINTLN("=== Config Received ===");
  DEBUG_PRINT("soundFont: ");        DEBUG_PRINTLN(web_soundFont);
  DEBUG_PRINT("volume: ");           DEBUG_PRINTLN(web_volume);
  DEBUG_PRINT("swingSensitivity: "); DEBUG_PRINTLN(web_swingSensitivity);
  DEBUG_PRINT("mainColor: ");        DEBUG_PRINTLN(web_mainColor);
  DEBUG_PRINT("blastColor: ");       DEBUG_PRINTLN(web_blastColor);
  DEBUG_PRINT("clashColor: ");       DEBUG_PRINTLN(web_clashColor);
  DEBUG_PRINT("customColor1: ");     DEBUG_PRINTLN(web_customColor1);
  DEBUG_PRINT("customColor2: ");     DEBUG_PRINTLN(web_customColor2);
  DEBUG_PRINT("customColor3: ");     DEBUG_PRINTLN(web_customColor3);

  // Apply to live variables
  soundFont        = web_soundFont;
  dfplayer_volume  = web_volume;
  swingSensitivity = web_swingSensitivity;
  MainColor        = static_cast<lightsaberColor>(web_mainColor);
  ClashColor       = static_cast<lightsaberColor>(web_clashColor);
  BlastColor       = static_cast<lightsaberColor>(web_blastColor);
  lightsaberColorHex[18] = web_customColor1;
  lightsaberColorHex[19] = web_customColor2;
  lightsaberColorHex[20] = web_customColor3;

  // Persist to NVS
  DEBUG_PRINTLN("Saving new Config");
  preferences.begin("Lightsaber", false);
  preferences.putUChar("SoundFont",         soundFont);
  preferences.putUChar("Volume",            dfplayer_volume);
  preferences.putUShort("SwingSensitivity", swingSensitivity);
  preferences.putUChar("MainColor",         static_cast<uint8_t>(MainColor));
  preferences.putUChar("ClashColor",        static_cast<uint8_t>(ClashColor));
  preferences.putUChar("BlastColor",        static_cast<uint8_t>(BlastColor));
  preferences.putUInt("UserColor1",         web_customColor1);
  preferences.putUInt("UserColor2",         web_customColor2);
  preferences.putUInt("UserColor3",         web_customColor3);
  preferences.end();

  audio.setVolume();
}
