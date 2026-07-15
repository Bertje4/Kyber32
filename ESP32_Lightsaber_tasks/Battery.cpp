#include <Esp.h>
#include "Battery.h"
#include "pinConfig.h"
#include "globalVariables.h"

bool battery_ready = false;

// Consumed by DFPlayer: when true, DFPlayer plays the critical sound and
// triggers retraction before the task resets this flag.
volatile bool battery_critical_shutdown = false;

extern global_states        global_state;
extern lightsaber_on_states lightsaber_on_state;
extern config_states        config_state;
extern bool                 configChanged;

BatteryMonitor::BatteryMonitor() {}

void BatteryMonitor::startTask() {
  xTaskCreatePinnedToCore(
    runTask,
    "BatteryTask",
    BATTERY_TASK_STACK_SIZE,
    this,
    BATTERY_TASK_PRIORITY,
    NULL,
    1);
}

void BatteryMonitor::runTask(void* pvParameters) {
  static_cast<BatteryMonitor*>(pvParameters)->BatteryCode();
}

void BatteryMonitor::initBattery() {
  // ADC_11db gives the full 0–3.3 V input range on ESP32.
  analogSetAttenuation(ADC_11db);
  pinMode(BATTERY_PIN, INPUT);
  DEBUG_PRINTLN("Battery monitor ready");
}

void BatteryMonitor::BatteryCode() {
  DEBUG_PRINT("BatteryTask running on core ");
  DEBUG_PRINTLN(xPortGetCoreID());

  TickType_t xLastWakeTime;
  const TickType_t xFrequency = pdMS_TO_TICKS(1000 / BATTERY_HZ);

  initBattery();
  battery_ready = true;

  // Seed with an initial reading so getLevel() is valid immediately.
  current_voltage_mV = readVoltage_mV();
  current_level      = voltageToLevel(current_voltage_mV);

  xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    current_voltage_mV = readVoltage_mV();
    battery_level new_level = voltageToLevel(current_voltage_mV);

    if (new_level != current_level) {
      current_level = new_level;
      DEBUG_PRINT("Battery level changed: ");
      DEBUG_PRINTLN(current_level);

      // ── Critical shutdown ─────────────────────────────────────────────────
      // Signal DFPlayer to play the warning and force retraction.
      // Only trigger while the saber is actually on to avoid spurious shutdowns
      // at boot when the ADC may not have settled yet.
      if (current_level == battery_critical && global_state == lightsaber_on) {
        DEBUG_PRINTLN("BATTERY CRITICAL — requesting shutdown");
        battery_critical_shutdown = true;
      }

      // ── Config-menu display ───────────────────────────────────────────────
      if (global_state == lightsaber_config && config_state == config_batteryLevel) {
        configChanged = true;
      }
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ── Helpers ──────────────────────────────────────────────────────────────────

uint32_t BatteryMonitor::readVoltage_mV() {
  uint32_t sum = 0;
  for (int i = 0; i < BATTERY_SAMPLES; i++) {
    sum += analogRead(BATTERY_PIN);
  }
  uint32_t avg     = sum / BATTERY_SAMPLES;
  uint32_t pin_mV  = (avg * ADC_REF_MV) / ADC_MAX_VALUE;
  uint32_t bat_mV  = (uint32_t)((float)pin_mV * BATTERY_DIVIDER_RATIO);

  DEBUG_PRINT("Battery ADC avg: ");
  DEBUG_PRINT(avg);
  DEBUG_PRINT("  pin_mV: ");
  DEBUG_PRINT(pin_mV);
  DEBUG_PRINT("  bat_mV: ");
  DEBUG_PRINTLN(bat_mV);

  return bat_mV;
}

battery_level BatteryMonitor::voltageToLevel(uint32_t voltage_mV) {
  if (voltage_mV >= BATTERY_FULL_MV)       return battery_full;
  if (voltage_mV >= BATTERY_NOMINAL_MV)    return battery_nominal;
  if (voltage_mV >= BATTERY_DIMINISHED_MV) return battery_diminished;
  if (voltage_mV >= BATTERY_LOW_MV)        return battery_low;
  if (voltage_mV >= BATTERY_CRITICAL_MV)   return battery_critical;
  return battery_critical;  // below critical floor
}

battery_level BatteryMonitor::getLevel()       { return current_level; }
uint32_t      BatteryMonitor::getVoltage_mV()  { return current_voltage_mV; }

uint8_t BatteryMonitor::getPercent() {
  if (current_voltage_mV >= BATTERY_FULL_MV)     return 100;
  if (current_voltage_mV <= BATTERY_CRITICAL_MV) return 0;
  uint32_t range = BATTERY_FULL_MV - BATTERY_CRITICAL_MV;
  uint32_t above = current_voltage_mV - BATTERY_CRITICAL_MV;
  return (uint8_t)((above * 100) / range);
}
