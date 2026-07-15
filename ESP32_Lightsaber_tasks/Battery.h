#ifndef BATTERY_H
#define BATTERY_H

#include <Esp.h>
#include "globalVariables.h"
#include "pinConfig.h"

// ── Voltage thresholds for a single 18650 cell (millivolts) ──────────────────
#define BATTERY_FULL_MV        4200
#define BATTERY_NOMINAL_MV     3800
#define BATTERY_DIMINISHED_MV  3500
#define BATTERY_LOW_MV         3300
#define BATTERY_CRITICAL_MV    3100

// ADC averaging — number of samples taken per reading
#define BATTERY_SAMPLES 16

// ── Voltage-divider ratio ─────────────────────────────────────────────────────
// Circuit:   BAT+ ── R1 ──┬── ESP ADC pin
//                         R2
//                        GND
//
// V_adc = V_bat * R2 / (R1 + R2)
// V_bat = V_adc * (R1 + R2) / R2
//
// IMPORTANT: use explicit float casts — R1 and R2 are integer #defines, so
//   plain integer arithmetic would give 100000/(100000+100000) = 0 (div by zero).
#define BATTERY_DIVIDER_RATIO  ((float)(R1 + R2) / (float)R2)

// ESP32 ADC reference voltage (mV) and resolution
#define ADC_REF_MV    3300
#define ADC_MAX_VALUE 4095

// ── Battery level enum ───────────────────────────────────────────────────────
enum battery_level {
  battery_full,
  battery_nominal,
  battery_diminished,
  battery_low,
  battery_critical,
  battery_unknown
};

// ── BatteryMonitor class ─────────────────────────────────────────────────────
class BatteryMonitor {
private:
  static void    runTask(void* pvParameters);
  void           BatteryCode();
  void           initBattery();
  uint32_t       readVoltage_mV();
  battery_level  voltageToLevel(uint32_t voltage_mV);

  battery_level  current_level      = battery_unknown;
  uint32_t       current_voltage_mV = 0;

public:
  BatteryMonitor();
  void startTask();

  battery_level getLevel();
  uint32_t      getVoltage_mV();
  uint8_t       getPercent();   // 0–100 linear estimate
};

// ── Globals declared here, defined in Battery.cpp ────────────────────────────
extern bool          battery_ready;
extern BatteryMonitor battery;

// Flag set by BatteryMonitor, consumed by DFPlayer to trigger shutdown sequence
extern volatile bool battery_critical_shutdown;

#endif  // BATTERY_H
