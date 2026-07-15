#include <Esp.h>
#include <FastLED.h>
#include "LEDs.h"
#include "pinConfig.h"
#include "globalVariables.h"

bool leds_ready = false;

extern global_states global_state;
extern lightsaber_on_states lightsaber_on_state;
extern config_states config_state;

lightsaberColor MainColor = Silver_blue;
lightsaberColor BlastColor = Sky_Blue;
lightsaberColor ClashColor = Pink_red;

uint8_t effectLeds = 0;
uint8_t effectLedsLength = 10;

// This array order must match the lightsaberColor enum in globalVariables.h
uint32_t lightsaberColorHex[] = {
  0x6464C8,  // Silver_blue
  0x969696,  // White       (150 instead of 255 saves power, still bright)
  0xFF0505,  // Pink_red    (aiming for ANH Vader colour)
  0xFF0000,  // Red
  0xFF0F00,  // Blood_Orange
  0xFF1E00,  // Orange
  0xFFB300,  // Gold
  0xFFFF00,  // Yellow      (can appear slightly green depending on setup)
  0xB3FF00,  // Neon_Green
  0x46FF00,  // Lime
  0x00FF00,  // Green
  0x00FF3C,  // Mint_Green
  0x00FF8C,  // Cyan
  0x008CFF,  // Sky_Blue
  0x0000FF,  // Blue
  0x7300FF,  // Purple
  0xDC00FF,  // Magenta
  0xABCDEF,  // Rainbow
  0xC0FF00,  // UserColor1
  0x809BCE,  // UserColor2
  0xF19953   // UserColor3
};

// ---------------------------------------------------------------------------
// Helper: mirror index for the return strip in series wiring.
//
// Physical layout (series):
//
//   Hilt                              Tip
//   [0] --------------->---------- [NUM_LEDS-1]        ← strip going up
//                                 | [NUM_LEDS]         ← tip LED (turnaround)
//   [2*NUM_LEDS-1] <-------------- [NUM_LEDS]          ← strip going down
//
// So LED i (0-based, going up) mirrors to index (2*NUM_LEDS - 1 - i).
// ---------------------------------------------------------------------------
#ifdef STRIPS_IN_SERIES
  static inline int mirrorIdx(int i) { return (2 * NUM_LEDS - 1) - i; }
#endif

// ---------------------------------------------------------------------------
Blade::Blade() {
  // Array sizes are fixed at compile time via TOTAL_LEDS in the header.
  // Nothing to do here.
}

void Blade::startTask() {
  xTaskCreatePinnedToCore(
    runTask,
    "LEDTask",
    LED_TASK_STACK_SIZE,
    this,
    LED_TASK_PRIORITY,
    NULL,
    1);
}

void Blade::runTask(void* pvParameters) {
  Blade* instance = static_cast<Blade*>(pvParameters);
  instance->LEDCode();
}

// ---------------------------------------------------------------------------
void Blade::initLEDS() {
  // Register the full strip length with FastLED so it drives every LED.
  FastLED.addLeds<WS2811, LED_OUTPUT, GRB>(leds_output_array, TOTAL_LEDS)
         .setCorrection(TypicalLEDStrip);

#ifdef CRYSTAL
  FastLED.addLeds<WS2811, CRYSTAL_LED_OUTPUT, GRB>(crystal_output_array, NUM_LEDS_CRYSTAL)
         .setCorrection(TypicalLEDStrip);
#endif

  FastLED.setBrightness(SABER_BRIGHTNESS);
}

// ---------------------------------------------------------------------------
void Blade::LEDCode() {
  DEBUG_PRINT("LEDTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());

  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / LEDS_HZ);

  initLEDS();
  leds_ready = true;

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {

    if (global_state == lightsaber_on) {
      switch (lightsaber_on_state) {

        // ── Ignition ──────────────────────────────────────────────────────
        case lightsaber_on_ignition:
#if NUM_LEDS_CRYSTAL > 0
          if (MainColor == Rainbow) {
            fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
          } else {
            fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
          }
#endif
          for (int i = 0; i < NUM_LEDS; i++) {
            if (MainColor == Rainbow) {
              fillRainbowLEDs(leds_output_array, i, colorNoiseSeed);
              colorNoiseSeed += colorNoiseSpeed;
            } else {
              fillFlickerLEDs(leds_output_array, i, CRGB(lightsaberColorHex[MainColor]), colorNoiseSeed, colorNoiseSpeed);
            }
            FastLED.show();
            vTaskDelay((BLADE_IGNITION_MS / NUM_LEDS) / portTICK_PERIOD_MS);
          }
          lightsaber_on_state = lightsaber_on_hum;
          break;

        // ── Retraction ────────────────────────────────────────────────────
        case lightsaber_on_retraction:
#if NUM_LEDS_CRYSTAL > 0
          fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB::Black);
#endif
          for (int i = NUM_LEDS - 1; i >= 0; i--) {
            if (MainColor == Rainbow) {
              fillRainbowLEDs(leds_output_array, i, colorNoiseSeed);
              colorNoiseSeed += colorNoiseSpeed;
            } else {
              fillFlickerLEDs(leds_output_array, i, CRGB(lightsaberColorHex[MainColor]), colorNoiseSeed, colorNoiseSpeed);
            }
            // Black out LEDs from i upward (tip side) on both strips
            for (int j = i; j < NUM_LEDS; j++) {
              leds_output_array[j] = CRGB::Black;
#ifdef STRIPS_IN_SERIES
              leds_output_array[mirrorIdx(j)] = CRGB::Black;
#endif
            }
            FastLED.show();
            vTaskDelay((BLADE_RETRACTION_MS / NUM_LEDS) / portTICK_PERIOD_MS);
          }
          global_state = lightsaber_idle;
          lightsaber_on_state = lightsaber_on_idle;
          break;

        // ── Blaster deflect ───────────────────────────────────────────────
        case lightsaber_on_blasterdeflect:
          if (MainColor == Rainbow) {
            fill_rainbow(leds_output_array, TOTAL_LEDS, colorNoiseSeed, 255 / TOTAL_LEDS);
#if NUM_LEDS_CRYSTAL > 0
          fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
          colorNoiseSeed += colorNoiseSpeed;
        } else {
#if NUM_LEDS_CRYSTAL > 0
          fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
#endif
            setLedsWithFlicker(MainColor);
            DEBUG_PRINTLN("BLASTER LEDS");
          }
          addBlasterToLeds();
          FastLED.show();
          break;

        // ── Blade lockup ──────────────────────────────────────────────────
        case lightsaber_on_bladelockup:
          if (MainColor == Rainbow) {
#if NUM_LEDS_CRYSTAL > 0
            fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
            colorNoiseSeed += colorNoiseSpeed;
          } else {
#if NUM_LEDS_CRYSTAL > 0
            fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
#endif
          }
          setLedsToLockup();
          FastLED.show();
          break;

        // ── Tip melt ──────────────────────────────────────────────────────
        case lightsaber_on_tipmelt:
          if (MainColor == Rainbow) {
#if NUM_LEDS_CRYSTAL > 0
            fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
            colorNoiseSeed += colorNoiseSpeed;
          } else {
#if NUM_LEDS_CRYSTAL > 0
            fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
#endif
          }
          setLedsToTipmelt();
          FastLED.show();
          break;

        // ── Clash ─────────────────────────────────────────────────────────
        case lightsaber_on_clash:
          colorNoiseSeed += colorNoiseSpeed;
          if (MainColor == Rainbow) {
            fill_rainbow(leds_output_array, TOTAL_LEDS, colorNoiseSeed, 255 / TOTAL_LEDS);
#if NUM_LEDS_CRYSTAL > 0
            fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
          } else {
            setSolidColor(CRGB(lightsaberColorHex[MainColor]));
          }
          addClashToLeds();
          FastLED.show();
          break;

        // ── Swing / Hum ───────────────────────────────────────────────────
        case lightsaber_on_swing:
        case lightsaber_on_hum:
          if (MainColor == Rainbow) {
            fill_rainbow(leds_output_array, TOTAL_LEDS, colorNoiseSeed, 255 / TOTAL_LEDS);
#if NUM_LEDS_CRYSTAL > 0
            fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
            colorNoiseSeed += colorNoiseSpeed;
          } else {
            setLedsWithFlicker(MainColor);
#if NUM_LEDS_CRYSTAL > 0
            fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
#endif
          }
          FastLED.show();
          break;

        default:
          setSolidColor(CRGB::Black);
          break;
      }

    } else if (global_state == lightsaber_config) {
      switch (config_state) {
        case config_maincolor:
          setColorOrRainbow(MainColor);
          FastLED.show();
          break;
        case config_clashcolor:
          setColorOrRainbow(ClashColor);
          FastLED.show();
          break;
        case config_blastcolor:
          setColorOrRainbow(BlastColor);
          FastLED.show();
          break;
        default:
          setSolidColor(CRGB::Black);
          break;
      }

    } else {
      // Idle state: crystal on, blade off
      fill_solid(leds_output_array, TOTAL_LEDS, CRGB::Black);
#if NUM_LEDS_CRYSTAL > 0
      fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, CRGB(lightsaberColorHex[MainColor]));
#endif
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ---------------------------------------------------------------------------
// setSolidColor — fills the entire strip (both halves in series mode)
// ---------------------------------------------------------------------------
void Blade::setSolidColor(CRGB color) {
  fill_solid(leds_output_array, TOTAL_LEDS, color);
#if NUM_LEDS_CRYSTAL > 0
  fill_solid(crystal_output_array, NUM_LEDS_CRYSTAL, color);
#endif
  FastLED.show();
}

// ---------------------------------------------------------------------------
// setLedsWithFlicker — flicker across the up-strip; mirror to down-strip
// ---------------------------------------------------------------------------
void Blade::setLedsWithFlicker(lightsaberColor color) {
  CRGB base = CRGB(lightsaberColorHex[color]);

  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t noise   = inoise8(i * 10, colorNoiseSeed);
    uint8_t flicker = map(noise, 0, 255, 108, 255);

    leds_output_array[i] = base;
    leds_output_array[i].nscale8_video(flicker);

#ifdef STRIPS_IN_SERIES
    leds_output_array[mirrorIdx(i)] = base;
    leds_output_array[mirrorIdx(i)].nscale8_video(flicker);
#endif

    colorNoiseSeed += colorNoiseSpeed;
  }
}

// ---------------------------------------------------------------------------
// addClashToLeds — pulsating clash overlay across the full strip
// ---------------------------------------------------------------------------
void Blade::addClashToLeds() {
  uint8_t phase = sin8((colorNoiseSeed * 255) / pulseInterval);
  CRGB clashColor = CRGB(lightsaberColorHex[ClashColor]);

  for (int i = 0; i < TOTAL_LEDS; i++) {
    leds_output_array[i] = blend(leds_output_array[i], clashColor, phase);
  }
}

// ---------------------------------------------------------------------------
// addBlasterToLeds — blaster spot on the up-strip; mirrored on down-strip
// ---------------------------------------------------------------------------
void Blade::addBlasterToLeds() {
  CRGB base = CRGB(lightsaberColorHex[BlastColor]);

  for (int i = effectLeds - effectLedsLength; i < effectLeds; i++) {
    uint8_t noise   = inoise8(i * 10, colorNoiseSeed);
    uint8_t flicker = map(noise, 0, 255, 108, 255);

    leds_output_array[i] = base;
    leds_output_array[i].nscale8_video(flicker);

#ifdef STRIPS_IN_SERIES
    leds_output_array[mirrorIdx(i)] = base;
    leds_output_array[mirrorIdx(i)].nscale8_video(flicker);
#endif
  }
}

// ---------------------------------------------------------------------------
// setLedsToLockup — random lockup flash across the full blade
// ---------------------------------------------------------------------------
void Blade::setLedsToLockup() {
  CRGB mainColor  = CRGB(lightsaberColorHex[MainColor]);
  CRGB clashColor = CRGB(lightsaberColorHex[ClashColor]);

  for (int i = 0; i < NUM_LEDS; i++) {
    int lockupFlick = random(0, 100);
    CRGB chosen;
    if (lockupFlick < 80) {
      chosen = (MainColor == Rainbow)
                 ? CRGB(CHSV(colorNoiseSeed + (i * (255 / NUM_LEDS)), 255, 255))
                 : mainColor;
    } else if (lockupFlick < 90) {
      chosen = clashColor;
    } else {
      chosen = CRGB::White;
    }

    uint8_t noise   = inoise8(i * 10, colorNoiseSeed);
    uint8_t flicker = map(noise, 0, 255, 108, 255);

    leds_output_array[i] = chosen;
    leds_output_array[i].nscale8_video(flicker);

#ifdef STRIPS_IN_SERIES
    leds_output_array[mirrorIdx(i)] = chosen;
    leds_output_array[mirrorIdx(i)].nscale8_video(flicker);
#endif
  }
}

// ---------------------------------------------------------------------------
// setLedsToTipmelt — normal blade + special tip effect
// ---------------------------------------------------------------------------
void Blade::setLedsToTipmelt() {
  CRGB mainColor  = CRGB(lightsaberColorHex[MainColor]);
  CRGB clashColor = CRGB(lightsaberColorHex[ClashColor]);

  // Fill base blade first
  if (MainColor == Rainbow) {
    fill_rainbow(leds_output_array, TOTAL_LEDS, colorNoiseSeed, 255 / TOTAL_LEDS);
  } else {
    setLedsWithFlicker(MainColor);
  }

  // Overwrite tip LEDs (and their mirrors) with the melt effect
  for (int i = NUM_LEDS - TIPMELT_LEDS; i < NUM_LEDS; i++) {
    int lockupFlick = random(0, 39);
    CRGB chosen;
    if (lockupFlick < 20 && MainColor != Rainbow) {
      chosen = mainColor;
    } else if (lockupFlick < 30) {
      chosen = clashColor;
    } else {
      chosen = CRGB::White;
    }

    uint8_t noise   = inoise8(i * 10, colorNoiseSeed);
    uint8_t flicker = map(noise, 0, 255, 108, 255);

    leds_output_array[i] = chosen;
    leds_output_array[i].nscale8_video(flicker);

#ifdef STRIPS_IN_SERIES
    leds_output_array[mirrorIdx(i)] = chosen;
    leds_output_array[mirrorIdx(i)].nscale8_video(flicker);
#endif
  }
}

// ---------------------------------------------------------------------------
// setColorOrRainbow — config menu preview
// ---------------------------------------------------------------------------
void Blade::setColorOrRainbow(lightsaberColor color) {
  if (color == Rainbow) {
    fill_rainbow(leds_output_array, TOTAL_LEDS, colorNoiseSeed, 255 / TOTAL_LEDS);
#if NUM_LEDS_CRYSTAL > 0
    fill_rainbow(crystal_output_array, NUM_LEDS_CRYSTAL, colorNoiseSeed, 255 / NUM_LEDS_CRYSTAL);
#endif
    colorNoiseSeed += colorNoiseSpeed;
  } else {
    setSolidColor(CRGB(lightsaberColorHex[color]));
  }
}

// ---------------------------------------------------------------------------
// fillRainbowLEDs — fill [0..count-1] with rainbow; mirror in series mode.
// Mirroring is handled here so call sites don't need to duplicate it.
// ---------------------------------------------------------------------------
void Blade::fillRainbowLEDs(CRGB* leds, int count, uint16_t baseHue) {
  for (int j = 0; j < count; j++) {
    uint8_t hue = baseHue + (j * (255 / NUM_LEDS));
    leds[j] = CHSV(hue, 255, 255);
#ifdef STRIPS_IN_SERIES
    leds[mirrorIdx(j)] = CHSV(hue, 255, 255);
#endif
  }
}

// ---------------------------------------------------------------------------
// fillFlickerLEDs — fill [0..count-1] with flickering color; mirror in series mode.
// ---------------------------------------------------------------------------
void Blade::fillFlickerLEDs(CRGB* leds, int count, CRGB color, uint16_t& seed, uint16_t speed) {
  for (int j = 0; j < count; j++) {
    uint8_t noise   = inoise8(j * 10, seed);
    uint8_t flicker = map(noise, 0, 255, 108, 255);
    leds[j] = color;
    leds[j].nscale8_video(flicker);
#ifdef STRIPS_IN_SERIES
    leds[mirrorIdx(j)] = color;
    leds[mirrorIdx(j)].nscale8_video(flicker);
#endif
    seed += speed;
  }
}
