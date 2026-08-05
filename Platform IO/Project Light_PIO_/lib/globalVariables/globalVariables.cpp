#include "globalVariables.h"

volatile bool pendingSave = false;
volatile bool pendingVolumeChange = true;

const char* globalStateToString(global_states state) {
  switch (state) {
    case lightsaber_idle:      return "lightsaber_idle";
    case lightsaber_on:        return "lightsaber_on";
    case lightsaber_config:    return "lightsaber_config";
    case lightsaber_charging:  return "lightsaber_charging";
    default:                   return "UNKNOWN_GLOBAL_STATE";
  }
}

const char* onStateToString(lightsaber_on_states state) {
  switch (state) {
    case lightsaber_on_hum:             return "lightsaber_on_hum";
    case lightsaber_on_ignition:        return "lightsaber_on_ignition";
    case lightsaber_on_retraction:      return "lightsaber_on_retraction";
    case lightsaber_on_bladelockup:     return "lightsaber_on_bladelockup";
    case lightsaber_on_tipmelt:         return "lightsaber_on_tipmelt";
    case lightsaber_on_blasterdeflect:  return "lightsaber_on_blasterdeflect";
    case lightsaber_on_clash:           return "lightsaber_on_clash";
    case lightsaber_on_swing:           return "lightsaber_on_swing";
    case lightsaber_on_idle:            return "lightsaber_on_idle";
    case lightsaber_on_web_config:      return "lightsaber_on_web_config";
    case lightsaber_on_boot:            return "lightsaber_on_boot";
    default:                            return "UNKNOWN_ON_STATE";
  }
}

const char* configStateToString(config_states state) {
  switch (state) {
    case config_idle:              return "config_idle";
    case config_soundfont:         return "config_soundfont";
    case config_volume:            return "config_volume";
    case config_swingsensitivity:  return "config_swingsensitivity";
    case config_maincolor:         return "config_maincolor";
    case config_clashcolor:        return "config_clashcolor";
    case config_blastcolor:        return "config_blastcolor";
    case config_batteryLevel:      return "config_batteryLevel";
    case config_lastMember:        return "config_lastMember";
    default:                       return "UNKNOWN_CONFIG_STATE";
  }
}

const char* lightsaberSoundToString(lightsaber_sounds sound) {
  switch (sound) {
    case sound_boot:       return "sound_boot";
    case sound_poweron:    return "sound_poweron";
    case sound_poweroff:   return "sound_poweroff";
    case sound_swing:      return "sound_swing";
    case sound_clash:      return "sound_clash";
    case sound_lockup:     return "sound_lockup";
    case sound_blaster:    return "sound_blaster";
    case sound_font:       return "sound_font";
    case sound_hum:        return "sound_hum";
    case sound_config:     return "sound_config";
    case sound_unknown:    return "sound_unknown";
    default:               return "UNKNOWN_LIGHTSABER_SOUND";
  }
}

const char* configSoundToString(config_sounds sound) {
  switch (sound) {
    case config_sound_up:                       return "config_sound_up";
    case config_sound_down:                     return "config_sound_down";
    case config_sound_configmode:               return "config_sound_configmode";
    case config_sound_Volume:                   return "config_sound_Volume";
    case config_sound_Soundfont:                return "config_sound_Soundfont";
    case config_sound_MainColor:                return "config_sound_MainColor";
    case config_sound_ClashColor:               return "config_sound_ClashColor";
    case config_sound_BlastColor:               return "config_sound_BlastColor";
    case config_sound_max:                      return "config_sound_max";
    case config_sound_min:                      return "config_sound_min";
    case config_sound_diyinolighsaber_boot_t2s: return "config_sound_diyinolighsaber_boot_t2s";
    case config_sound_yes:                      return "config_sound_yes";
    case config_sound_no:                       return "config_sound_no";
    case config_sound_diyinojukebox_t2s:        return "config_sound_diyinojukebox_t2s";
    case config_sound_batterynominal:           return "config_sound_batterynominal";
    case config_sound_batterydiminished:        return "config_sound_batterydiminished";
    case config_sound_batterylow:               return "config_sound_batterylow";
    case config_sound_batterycritical:          return "config_sound_batterycritical";
    case config_sound_batteryfull:              return "config_sound_batteryfull";
    case config_sound_bladetype:                return "config_sound_bladetype";
    case config_sound_ledstringblade:           return "config_sound_ledstringblade";
    case config_sound_pixelblade:               return "config_sound_pixelblade";
    case config_sound_starledblade:             return "config_sound_starledblade";
    case config_sound_ignitionstyle:            return "config_sound_ignitionstyle";
    case config_sound_flickerStyle:             return "config_sound_flickerStyle";
    case config_sound_swingsensitivity:         return "config_sound_swingsensitivity";
    case config_sound_programmingMode:          return "config_sound_programmingMode";
    case config_sound_storageMediaAccess:       return "config_sound_storageMediaAccess";
    case config_sound_sleepModeInit:            return "config_sound_sleepModeInit";
    case config_sound_unknown:                  return "config_sound_unknown";
    default:                                    return "UNKNOWN_CONFIG_SOUND";
  }
}