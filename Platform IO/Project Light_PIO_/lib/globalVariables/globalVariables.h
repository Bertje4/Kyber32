#ifndef GLOBALVARIABLES_H
#define GLOBALVARIABLES_H

extern volatile bool pendingSave;
extern volatile bool pendingVolumeChange;

enum global_states {
  lightsaber_idle,
  lightsaber_on,
  lightsaber_config,
  lightsaber_charging   // charger plugged in — all subsystems halted except charge monitor
};

enum lightsaber_on_states {
  lightsaber_on_hum,
  lightsaber_on_ignition,
  lightsaber_on_retraction,
  lightsaber_on_bladelockup,
  lightsaber_on_tipmelt,
  lightsaber_on_blasterdeflect,
  lightsaber_on_clash,
  lightsaber_on_swing,
  lightsaber_on_idle,
  lightsaber_on_web_config,
  lightsaber_on_boot
};

enum config_states {
  config_idle,
  config_soundfont,
  config_volume,
  config_swingsensitivity,
  config_maincolor,
  config_clashcolor,
  config_blastcolor,
  config_batteryLevel,
  config_lastMember
};

//Colors from https://www.reddit.com/r/lightsabers/comments/w5m02g/xenopixel_rgb_color_codes/
enum lightsaberColor {
  Silver_blue,
  White,     //(having the values at 150 instead of 255 saves some power and is still bright)
  Pink_red,  //(aiming for ANH Vader colour)
  Red,
  Blood_Orange,
  Orange,
  Gold,
  Yellow,  //(can appear slightly green depending on setup - see more below)
  Neon_Green,
  Lime,
  Green,
  Mint_Green,
  Cyan,
  Sky_Blue,
  Blue,
  Purple,
  Magenta,
  Rainbow,
  UserColor1,
  UserColor2,
  UserColor3,
  NumColors
};

enum lightsaber_sounds {
  sound_boot,
  sound_poweron,
  sound_poweroff,
  sound_swing,
  sound_clash,
  sound_lockup,
  sound_blaster,
  sound_font,
  sound_hum,
  sound_config,
  sound_unknown
};

enum config_sounds {
  config_sound_up,
  config_sound_down,
  config_sound_configmode,
  config_sound_Volume,
  config_sound_Soundfont,
  config_sound_MainColor,
  config_sound_ClashColor,
  config_sound_BlastColor,
  config_sound_max,
  config_sound_min,
  config_sound_diyinolighsaber_boot_t2s,
  config_sound_yes,
  config_sound_no,
  config_sound_diyinojukebox_t2s,
  config_sound_batterynominal,
  config_sound_batterydiminished,
  config_sound_batterylow,
  config_sound_batterycritical,
  config_sound_batteryfull,
  config_sound_bladetype,
  config_sound_ledstringblade,
  config_sound_pixelblade,
  config_sound_starledblade,
  config_sound_ignitionstyle,
  config_sound_flickerStyle,
  config_sound_swingsensitivity,
  config_sound_programmingMode,
  config_sound_storageMediaAccess,
  config_sound_sleepModeInit,
  config_sound_unknown
};

const char* globalStateToString(global_states state);

const char* onStateToString(lightsaber_on_states state);

const char* configStateToString(config_states state);

const char* lightsaberSoundToString(lightsaber_sounds sound);

const char* configSoundToString(config_sounds sound);

#endif