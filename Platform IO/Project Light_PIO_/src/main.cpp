// this example will play a random track from all on the sd
//
// it expects the sd card to contain some mp3 files

#define FASTLED_RMT5 0   // fix for pin 27 with newer FastLED IDF5 driver

#include "Arduino.h"
#include "Preferences.h"
#include <DFMiniMp3.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "esp_task_wdt.h"

#include "pinConfig.h"
#include "globalVariables.h"
#include "buttons.h"
#include "LEDs.h"
#include "MPU.h"
#include "DFPlayer.h"
#include "configMenu.h"
#include "Battery.h"

// FreeRTOS Configuration for Runtime Stats
#define configGENERATE_RUN_TIME_STATS 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() (timer_start())  // Configure timer for runtime stats
#define portGET_RUN_TIME_COUNTER_VALUE() (millis())               // Use micros() for more precise time
#define TASK_START_DELAY 100

MovementDetection mpuClass;
Blade leds;
BatteryMonitor battery;

// FROM:
// DFPlayer audio(Serial1);

// TO:
static uint8_t audioBuf[sizeof(DFPlayer)] __attribute__((aligned(4)));
DFPlayer& audio = *reinterpret_cast<DFPlayer*>(audioBuf);

Buttons mainButton(button_double_main);
#ifndef SINGLE_BUTTON_MODE
  Buttons secondaryButton(button_double_secondary);
#endif

ConfigMenu menu = ConfigMenu();

bool print_stats = true;

// Serial mutex — defined here, declared extern in pinConfig.h
// Prevents interleaved debug output from multiple FreeRTOS tasks
SemaphoreHandle_t serial_mutex = NULL;  // ← explicit NULL, not just declared

global_states global_state = lightsaber_idle;
lightsaber_on_states lightsaber_on_state = lightsaber_on_boot;
config_states config_state = config_idle;

#ifndef ON_PLATFORM_IO
  void printTask(void* pvParameters) {
    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before printing stats
      printTaskStats();
    }
  }

  // Function to print FreeRTOS runtime stats
  void printTaskStats() {
    char buffer[512];
    vTaskGetRunTimeStats(buffer);  // Get task runtime stats into the buffer
    DEBUG_PRINTLN("\nFreeRTOS Task Run Time Stats:");
    DEBUG_PRINTLN(buffer);  // Print the stats to the Serial Monitor
  }
#endif

void setup() {
  esp_task_wdt_delete(NULL);  // remove watchdog from this task (setup/loop task)

  new (&audio) DFPlayer(Serial1);  // construct in place, NOW, after hardware init

  serial_mutex = xSemaphoreCreateMutex();  // must be first — DEBUG macros depend on it
  Serial.begin(115200);
  delay(8000);  // wait for USB CDC to enumerate before first print

  // ADD THESE LINES — tells you exactly what caused the reboot
  esp_reset_reason_t reason = esp_reset_reason();
  Serial.println();
  Serial.println("----> New Loop Running <----");
  Serial.print("Reset reason: ");
  Serial.println(reason);
  // 3 = SW reset, 4 = legacy watch dog, 5 = deep sleep, 
  // 6 = SLC reset, 7 = BROWNOUT, 8 = SDIO reset
  // 3 with looping = panic/exception
  
  Serial.print("Free heap: ");
  Serial.println(esp_get_free_heap_size());
  
  Serial.println();
  DEBUG_PRINTLN("=== setup() start ===");

  DEBUG_PRINTLN("setup: calling readConfig");
  menu.readConfig();
  DEBUG_PRINTLN("setup: readConfig done");

  // ── Early-boot charging check ─────────────────────────────────────────────
  // If the charger is already plugged in when we boot, enter charge-safe mode
  // immediately — before starting any other tasks (WiFi, LEDs, DFPlayer, etc.).
  // The battery task will detect the unplug and set global_state back to idle.
  {
    analogSetAttenuation(ADC_11db);
    pinMode(CHARGING_PIN, INPUT);
    // Take a quick averaged reading to avoid ADC noise triggering a false positive.
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogRead(CHARGING_PIN);
    uint32_t pin_mV = ((sum / 16) * 3300) / 4095;
    if (pin_mV >= CHARGING_DETECT_THRESHOLD_MV) {
      DEBUG_PRINTLN("setup: charger detected at boot — entering charge-safe mode");
      global_state = lightsaber_charging;
      charging_detected = true;
    }
  }

  DEBUG_PRINTLN("setup: starting battery task");
  battery.startTask();
  vTaskDelay(TASK_START_DELAY);
  DEBUG_PRINTLN("setup: battery task started");

  DEBUG_PRINTLN("setup: starting button task(s)");
  mainButton.startTask();
  
  #ifndef SINGLE_BUTTON_MODE
    secondaryButton.startTask();
  #endif
  vTaskDelay(TASK_START_DELAY);
  DEBUG_PRINTLN("setup: button task(s) started");

  DEBUG_PRINTLN("setup: starting MPU task");
  mpuClass.startTask();
  vTaskDelay(TASK_START_DELAY);
  DEBUG_PRINTLN("setup: MPU task started");

  DEBUG_PRINTLN("setup: starting LED task");
  leds.startTask();
  vTaskDelay(TASK_START_DELAY);
  DEBUG_PRINTLN("setup: LED task started");

  DEBUG_PRINTLN("setup: starting DFPlayer task");
  audio.startTask();
  vTaskDelay(TASK_START_DELAY);
  DEBUG_PRINTLN("setup: DFPlayer task started");

  #ifndef ON_PLATFORM_IO
    if (print_stats) {
      // Create a task to print runtime stats every second
      xTaskCreatePinnedToCore(
        printTask,    /* Task function. */
        "PrintStats", /* name of task. */
        4096,         /* Stack size of task */
        NULL,         /* parameter of the task */
        1,            /* priority of the task */
        NULL,         /* Task handle to keep track of created task */
        1);           /* pin task to core 1 */
    }
  #endif
}

void loop() {
  //Intentionally left empty as all functionality is done by other tasks
}
