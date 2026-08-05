#include <Esp.h>
#include <DFMiniMp3.h>
#include "esp_task_wdt.h"

#include "DFPlayer.h"
#include "pinConfig.h"
#include "globalVariables.h"
#include "configMenu.h"
#include "Battery.h"

bool dfplayer_ready = false;
extern bool leds_ready;
extern bool mpu_ready;
extern bool buttons_ready;

extern global_states global_state;
extern lightsaber_on_states lightsaber_on_state;
extern config_states config_state;

uint8_t soundFont = 1;
uint8_t dfplayer_volume = 1;

bool configChanged = false;
bool soundFontChanged = false;
bool configChangedUp = false;
bool configChangedDown = false;

extern ConfigMenu menu;

SemaphoreHandle_t config_mutex;

bool configStart = true;

volatile bool SoundFinishedPlaying = true;

DFPlayer::DFPlayer(HardwareSerial& serialPort)
  : dfmp3(serialPort) {
  // Any other initialization you need for MyPlayer
}

// Start the task by creating a FreeRTOS task
void DFPlayer::startTask() {
  // Create the task, passing `this` (the instance of the class) as the parameter
  #ifdef DUAL_CORE
    xTaskCreatePinnedToCore(
      runTask,        /* Task function. */
      "DFPlayerTask", /* name of task. */
      DFPLAYER_TASK_STACK_SIZE,           /* Stack size of task */
      this,           /* parameter of the task */
      DFPLAYER_TASK_PRIORITY,              /* priority of the task */
      &dfTaskHandle,  /* Task handle to keep track of created task */
      1 /* pin task to core 1 */
    );             
  #else
    xTaskCreate(
      runTask,        /* Task function. */
      "DFPlayerTask", /* name of task. */
      DFPLAYER_TASK_STACK_SIZE,           /* Stack size of task */
      this,           /* parameter of the task */
      DFPLAYER_TASK_PRIORITY,              /* priority of the task */
      &dfTaskHandle  /* Task handle to keep track of created task */
    ); 
  #endif
}

void DFPlayer::setVolume() {
  DEBUG_PRINTLN("***** [DFPlayer Mini]: SetVolume called! *****");

  if (SoundFinishedPlaying) {
    DEBUG_PRINT("[DFPlayer Mini]: Setting volume to: ");
    DEBUG_PRINT(dfplayer_volume);
    dfmp3.setVolume(dfplayer_volume);
    DEBUG_PRINTLN("[DFPlayer Mini]: Volume has been set!");

    // DEBUG_PRINTLN("[DFPlayer Mini]: Get volume disabled!");
    uint16_t volume = dfmp3.getVolume();
    DEBUG_PRINT("volume ");
    DEBUG_PRINTLN(volume);

    pendingVolumeChange = false;
    DEBUG_PRINTLN("[DFPlayer Mini]: setVolume done!");
  } else {
    DEBUG_PRINTLN("[DFPlayer Mini]: audio is still playing, I defer the volume call...");
  }
  DEBUG_PRINTLN(" ***** [DFPlayer Mini]: SetVolume ended! *****");
}

// Static task function called by FreeRTOS
void DFPlayer::runTask(void* pvParameters) {
  // Cast the parameter to a pointer to the MyTaskClass instance
  DFPlayer* instance = static_cast<DFPlayer*>(pvParameters);

  // Call the instance's non-static method (the actual task)
  instance->DFPlayerCode();
}

void DFPlayer::initDFPlayer() {
  DEBUG_PRINTLN("[DFPlayer Mini]: DFPlayer Mini init start");
  esp_task_wdt_delete(NULL);  // remove watchdog from this task

  config_mutex = xSemaphoreCreateMutex();
  if (config_mutex == NULL) {
    DEBUG_PRINTLN("[DFPlayer Mini]: Mutex not correctly created");
  }
  DEBUG_PRINTLN("[DFPlayer Mini]: Mutex oke");

  dfmp3.begin(/*rx =*/RX_DFPLAYER, /*tx =*/TX_DFPLAYER);
  // for boards that support hardware arbitrary pins
  // dfmp3.begin(10, 11); // RX, TX

  // during development, it's a good practice to put the module
  // into a known state by calling reset().
  // You may hear popping when starting and you can remove this
  // call to reset() once your project is finalized
  DEBUG_PRINTLN("[DFPlayer Mini]: Begin oke");

  dfmp3.reset();
  DEBUG_PRINTLN("[DFPlayer Mini]: Reset oke");

  uint16_t version = dfmp3.getSoftwareVersion();
  DEBUG_PRINT("[DFPlayer Mini]: Version ");
  DEBUG_PRINTLN(version);

  // show some properties and set the volume
  DEBUG_PRINT("[DFPlayer Mini]: Setting volume to: ");
  DEBUG_PRINTLN(dfplayer_volume);
  dfmp3.setVolume(dfplayer_volume);
  uint16_t volume = dfmp3.getVolume();
  DEBUG_PRINT("[DFPlayer Mini]: Volume ");
  DEBUG_PRINTLN(volume);

  uint16_t count = dfmp3.getTotalTrackCount(DfMp3_PlaySource_Sd);
  DEBUG_PRINT("[DFPlayer Mini]: Files ");
  DEBUG_PRINTLN(count);

  uint16_t mode = dfmp3.getPlaybackMode();
  DEBUG_PRINT("[DFPlayer Mini]: Playback mode ");
  DEBUG_PRINTLN(mode);

  DEBUG_PRINTLN("[DFPlayer Mini]: DFPlayer Mini started up succesfully!");
}

void DFPlayer::DFPlayerCode() {
  DEBUG_PRINT("DFPlayerTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());
  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS((1000 / DFPLAYER_HZ));

  initDFPlayer();

  dfplayer_ready = true;

  bool dfplayer_sleeping = false;  // tracks whether we sent a sleep command

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    // ── Charge-safe mode: sleep DFPlayer, block everything, poll for unplug ──
    if (global_state == lightsaber_charging) {
      if (!dfplayer_sleeping) {
        DEBUG_PRINTLN("[DFPlayer] Charger detected — stopping playback and sleeping module");
        dfmp3.stop();
        dfmp3.sleep();
        dfplayer_sleeping = true;
        current_sound = sound_unknown;
        current_config_sound = config_sound_unknown;
      }
      vTaskDelay(pdMS_TO_TICKS(500));
      // Reset wake time so we don't burst-catch-up when charging ends.
      xLastWakeTime = xTaskGetTickCount();
      continue;
    }

    // If we just came back from charging, wake the DFPlayer module.
    if (dfplayer_sleeping) {
      DEBUG_PRINTLN("[DFPlayer] Charger removed — waking module");
      dfmp3.reset();
      dfmp3.setVolume(dfplayer_volume);
      dfplayer_sleeping = false;
      firstBoot = true;  // re-play boot sound after charge
    }

    // DEBUG_PRINTLN("DFPLAYER Tick.");

    if (pendingSave) {
        DEBUG_PRINTLN("[DFPlayer Mini]: pendingSave detected, calling doSave");
        menu.doSave();
        // dfmp3.setVolume(dfplayer_volume);  // this would call setVolume a second time...
    }

    if (pendingVolumeChange) {
      setVolume();
    }

    // DEBUG_PRINTLN();
    // DEBUG_PRINT("[DFPlayer Mini]: globalstate = ");
    // DEBUG_PRINT(globalStateToString(global_state));

    if (global_state == lightsaber_on) {
      current_sound = getCurrentLightsaberTrack();
      
      // DEBUG_PRINT(" | current_sound = ");
      // DEBUG_PRINTLN(lightsaberSoundToString(current_sound));

      switch (lightsaber_on_state) {
        case lightsaber_on_ignition:
          if (current_sound != sound_poweron) {
            playLightsaberTrack(sound_poweron);
          }
          break;

        case lightsaber_on_retraction:
          if (current_sound != sound_poweroff) {
            playLightsaberTrack(sound_poweroff);
          }
          break;

        case lightsaber_on_clash:
          if (current_sound != sound_clash) {
            playLightsaberTrack(sound_clash);
          }
          break;

        case lightsaber_on_blasterdeflect:
          if (current_sound != sound_blaster) {
            playLightsaberTrack(sound_blaster);
          }
          break;

        case lightsaber_on_tipmelt:
          break;
        case lightsaber_on_bladelockup:
          if (current_sound != sound_lockup) {
            playLightsaberTrack(sound_lockup);
          }
          break;

        case lightsaber_on_swing:
          if (current_sound != sound_swing) {
            playLightsaberTrack(sound_swing);
          }
          break;

        case lightsaber_on_hum:
          if (current_sound != sound_hum) {
            loopLightsaberTrack(sound_hum);
          }
          break;

        default:
          dfmp3.stop();
          current_sound = sound_unknown;
          break;
      }
    } else if (global_state == lightsaber_config) {
      xSemaphoreTake(config_mutex, portMAX_DELAY);

      config_states switch_config_state = config_state;

      switch (switch_config_state) {
        case config_idle:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_configmode) {
            DEBUG_PRINTLN("Play config_sound_configmode");
            playConfigTrack(config_sound_configmode);
          }
          if (configStart) {
            DEBUG_PRINTLN("Playing config sound");
            configStart = false;
          } else {
            //Done playing boot sound
            if (dfmp3.getStatus().state == DfMp3_StatusState_Idle) {
              config_state = config_soundfont;
              configChanged = true;
            }
          }
          break;
        case config_soundfont:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_Soundfont && configChanged) {
            DEBUG_PRINTLN("Play config_sound_Soundfont");
            playConfigTrack(config_sound_Soundfont);
            configChanged = false;
          }
          if (soundFontChanged) {
            playLightsaberTrack(sound_font);
            soundFontChanged = false;
          }
          break;
        case config_volume:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_Volume && configChanged) {
            DEBUG_PRINTLN("Play config_sound_Volume");
            playConfigTrack(config_sound_Volume);
            configChanged = false;
          }
          if (configChangedUp) {
            playConfigTrack(config_sound_up);
            configChangedUp = false;
          }
          if (configChangedDown) {
            playConfigTrack(config_sound_down);
            configChangedDown = false;
          }
          break;
        case config_swingsensitivity:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_swingsensitivity && configChanged) {
            DEBUG_PRINTLN("Play config_sound_swingsensitivity");
            playConfigTrack(config_sound_swingsensitivity);
            configChanged = false;
          }
          if (configChangedUp) {
            playConfigTrack(config_sound_up);
            configChangedUp = false;
          }
          if (configChangedDown) {
            playConfigTrack(config_sound_down);
            configChangedDown = false;
          }
          break;
        case config_maincolor:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_MainColor && configChanged) {
            DEBUG_PRINTLN("Play config_sound_MainColor");
            playConfigTrack(config_sound_MainColor);
            configChanged = false;
          }
          if (configChangedUp) {
            playConfigTrack(config_sound_up);
            configChangedUp = false;
          }
          if (configChangedDown) {
            playConfigTrack(config_sound_down);
            configChangedDown = false;
          }
          break;
        case config_clashcolor:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_ClashColor && configChanged) {
            DEBUG_PRINTLN("Play config_sound_ClashColor");
            playConfigTrack(config_sound_ClashColor);
            configChanged = false;
          }
          if (configChangedUp) {
            playConfigTrack(config_sound_up);
            configChangedUp = false;
          }
          if (configChangedDown) {
            playConfigTrack(config_sound_down);
            configChangedDown = false;
          }
          break;
        case config_blastcolor:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_BlastColor && configChanged) {
            DEBUG_PRINTLN("Play config_sound_BlastColor");
            playConfigTrack(config_sound_BlastColor);
            configChanged = false;
          }
          if (configChangedUp) {
            playConfigTrack(config_sound_up);
            configChangedUp = false;
          }
          if (configChangedDown) {
            playConfigTrack(config_sound_down);
            configChangedDown = false;
          }
          break;
        case config_batteryLevel:
          current_config_sound = getCurrentconfigTrack();
          if (current_config_sound != config_sound_batterynominal && configChanged) {
            DEBUG_PRINTLN("Play config_sound_batterynominal");
            playConfigTrack(config_sound_batterynominal);
            configChanged = false;
          }
          break;
      }
      xSemaphoreGive(config_mutex);
    } else { //global state is lightsaber_idle
      if (lightsaber_on_state == lightsaber_on_boot) {
        current_sound = getCurrentLightsaberTrack();
        if (current_sound != sound_boot) {
          playLightsaberTrack(sound_boot);
          // need small delay to allow for bootsound to start playing
          vTaskDelay(pdMS_TO_TICKS(100));  // Convert milliseconds to FreeRTOS ticks
        }
        if (firstBoot) {
          DEBUG_PRINTLN("Playing boot sound");
          firstBoot = false;
        } else {
          //Done playing boot sound
          if (dfmp3.getStatus().state == DfMp3_StatusState_Idle) {
            lightsaber_on_state = lightsaber_on_idle;
          }
        }
      }
    }
    if (SoundFinishedPlaying) {
      dfmp3.stop(); // stop spamming the retraction sound!
    };

    dfmp3.loop();
    // Runs task every 20 MS
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void DFPlayer::playLightsaberTrack(lightsaber_sounds sound_to_play) {
  SoundFinishedPlaying = false;
  current_sound = sound_to_play;
  dfmp3.playGlobalTrack(fontAndEnumtoTrack(sound_to_play, soundFont));
}

void DFPlayer::loopLightsaberTrack(lightsaber_sounds sound_to_play) {
  SoundFinishedPlaying = false;
  current_sound = sound_to_play;
  dfmp3.loopGlobalTrack(fontAndEnumtoTrack(sound_to_play, soundFont));
}

lightsaber_sounds DFPlayer::getCurrentLightsaberTrack() {
  return current_sound;
  // return getEnumFromGlobalTrack(dfmp3.getCurrentTrack());
}

void DFPlayer::playConfigTrack(config_sounds sound_to_play) {
  SoundFinishedPlaying = false;
  current_config_sound = sound_to_play;
  dfmp3.playGlobalTrack(static_cast<int>(sound_to_play) + 1);
}

config_sounds DFPlayer::getCurrentconfigTrack() {
  uint16_t current_track = dfmp3.getCurrentTrack() - 1;
  if (current_track > static_cast<int>(config_sound_unknown)) {
    return config_sound_unknown;
  }
  return static_cast<config_sounds>(current_track);
}

uint16_t DFPlayer::fontAndEnumtoTrack(lightsaber_sounds sound, uint8_t soundFont) {
  uint8_t track = 0;
  switch (sound) {
    case sound_boot:
      track = 1;
      break;
    case sound_poweron:
      track = random(2, 5 + 1);
      break;
    case sound_poweroff:
      track = random(6, 7 + 1);
      break;
    case sound_swing:
      track = random(8, 15 + 1);
      break;
    case sound_clash:
      track = random(16, 23 + 1);
      break;
    case sound_lockup:
      track = 24;
      break;
    case sound_blaster:
      track = random(25, 28 + 1);
      break;
    case sound_font:
      track = 29;
      break;
    default:
    case sound_hum:
      track = 30;
      break;
  }
  return getGlobalTrackFromFolderandTrack(soundFont + 1, track);
}

uint16_t DFPlayer::getGlobalTrackFromFolderandTrack(uint8_t folderInt, uint8_t trackInt) {
  if (folderInt == 1) {
    return trackInt;
  } else {
    return (folderInt - 2) * 30 + 29 + trackInt;
  }
  return 0;
}

lightsaber_sounds DFPlayer::getEnumFromGlobalTrack(uint16_t globalTrackInt) {
  if (globalTrackInt < 29) {
    return sound_config;
  } else {
    uint8_t fontTrack = ((globalTrackInt - 29) - 1) % 30;
    fontTrack = fontTrack + 1;  // correct for annoying 1 to 30 counting
    if (fontTrack == 1) return sound_boot;
    if (2 <= fontTrack && fontTrack <= 5) return sound_poweron;
    if (6 <= fontTrack && fontTrack <= 7) return sound_poweroff;
    if (8 <= fontTrack && fontTrack <= 15) return sound_swing;
    if (16 <= fontTrack && fontTrack <= 23) return sound_clash;
    if (fontTrack == 24) return sound_lockup;
    if (25 <= fontTrack && fontTrack <= 28) return sound_blaster;
    if (fontTrack == 29) return sound_font;
    if (fontTrack == 30) return sound_hum;
  }
  return sound_unknown;
}

class DFPlayer::Mp3Notify {
public:
  static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action) {
    if (source & DfMp3_PlaySources_Sd) {
      DEBUG_PRINT("SD Card, ");
    }
    if (source & DfMp3_PlaySources_Usb) {
      DEBUG_PRINT("USB Disk, ");
    }
    if (source & DfMp3_PlaySources_Flash) {
      DEBUG_PRINT("Flash, ");
    }
    DEBUG_PRINTLN(action);
  }
  static void OnError([[maybe_unused]] DfMp3& mp3, uint16_t errorCode) {
    // see DfMp3_Error for code meaning
    DEBUG_PRINTLN();
    DEBUG_PRINT("[DFPlayer Mini]: Communication Error ");
    DEBUG_PRINTLN(errorCode);
  }
  static void OnPlayFinished([[maybe_unused]] DfMp3& mp3, [[maybe_unused]] DfMp3_PlaySources source, uint16_t track) {
    SoundFinishedPlaying = true;
    global_state = lightsaber_idle;
    DEBUG_PRINT("[DFPlayer Mini]: Play finished for #");
    DEBUG_PRINTLN(track);
  }
  static void OnPlaySourceOnline([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "online");
  }
  static void OnPlaySourceInserted([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "inserted");
  }
  static void OnPlaySourceRemoved([[maybe_unused]] DfMp3& mp3, DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "removed");
  }
};