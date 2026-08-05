#ifndef DFPLAYER_H
#define DFPLAYER_H

#include <Esp.h>
#include "Arduino.h"
#include "globalVariables.h"
#include <DFMiniMp3.h>

class DFPlayer {
private:
  // Code for sound creating objects
  class Mp3Notify;
  // define a handy type using serial and our notify class
  typedef DFMiniMp3<HardwareSerial, Mp3Notify> DfMp3;
  DfMp3 dfmp3;

  // Code for task creation and running
  static void runTask(void* pvParameters);
  void DFPlayerCode();

  // Code for helper functions in tasks
  void initDFPlayer();

  lightsaber_sounds getEnumFromGlobalTrack(uint16_t globalTrackInt);
  uint16_t getGlobalTrackFromFolderandTrack(uint8_t folderInt, uint8_t trackInt);

  void playLightsaberTrack(lightsaber_sounds sound_to_play);
  void loopLightsaberTrack(lightsaber_sounds sound_to_play);
  lightsaber_sounds getCurrentLightsaberTrack();

  void playConfigTrack(config_sounds sound_to_play);
  config_sounds getCurrentconfigTrack();


lightsaber_sounds current_sound = sound_unknown;
config_sounds current_config_sound = config_sound_unknown;
  TaskHandle_t dfTaskHandle = NULL;  // Declare a global task handle
  bool firstBoot = true;

public:
  // Code for sound creating objects
  DFPlayer(HardwareSerial& serialPort);

  // Code for task creation and running
  void startTask();
  void setVolume();

  // Code for helper functions in tasks
  uint16_t fontAndEnumtoTrack(lightsaber_sounds sound, uint8_t soundFont);
};

extern bool dfplayer_ready;

#endif