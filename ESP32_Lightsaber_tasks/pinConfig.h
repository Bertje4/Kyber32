#ifndef PINCONFIG_H
#define PINCONFIG_H

// Pin Definitions
#define RX_DFPLAYER 1   // 16
#define TX_DFPLAYER 3   //17

#define SINGLE_BUTTON_MODE  // comment this out to re-enable two-button mode

// If buttons aren't rgb, no problem, just connect the corresponding colour and leave the rest NOT wired
// the program defines them as outputs, so they can't be used for other tasks

#define MAIN_BUTTON 17  //32

// CHANGEME - Button LD wiring
#define MAIN_RED 23
#define MAIN_GREEN 19
#define MAIN_BLUE 18

#ifdef SINGLE_BUTTON_MODE
  #define LONG_PRESS_LOCKUP_MS 1000  // hold longer than this = lockup, shorter = retract
#endif

#ifndef SINGLE_BUTTON_MODE
  #define SECOND_BUTTON 25
  #define SECOND_RED 13
  #define SECOND_GREEN 33
  #define SECOND_BLUE 26
#endif

#define MPU_INTERRUPT 4

// #define CRYSTAL   // uncomment to enable crystal chamber LED

#ifdef CRYSTAL
  #define CRYSTAL_LED_OUTPUT 15
  #define NUM_LEDS_CRYSTAL 1
#else
  #define NUM_LEDS_CRYSTAL 0   // zero-length, never written to
#endif

#define LED_OUTPUT 13  // GPIO for data pin of NEOPixel Strip(s)

/*
The original library expects both LED strips (glued back to back for a fuller look) to be wired in parallel in the hilt (Figure 1).
This version also allows to wire the strips in series at the to of the blade -> arrows of the strip with connector go up the blade, where that
strip is soldered to the other one with the arrows facing the hilt, in effect creating one U-shaped strip going up and down the blade.
This configuration has the advantages that the code can individually adress all the LEDS (instead of both strips doing accictily the same), and
it allows for putting the LED at the top of te blade parallel to a cross-section of the blade, allowing for more light in the tip (FIGURE 2).
It his however important that -- when using the series configuration -- the total number of LEDS remains even AND equal op and down.
The code uses the horizontal LED as last LED of the strip going up the blade.

Graphical representation

  Hilt                                              Tip of blade


Figure 1: parallel

    -----------------------------------------------------> strip one  |
                                                                      |--> sharing the same data line at the bottom -> both strips act in accactely the same way
    -----------------------------------------------------> strip two  | 

Figure 2: series

    -----------------------------------------------------  "strip one"    |
                                                        |  "parallel LED" |--> forming one big strip going up and down the blade, with ONLY strip one being (directly) connected to the esp
    <----------------------------------------------------  "strip two"    |

*/
#define STRIPS_IN_SERIES // uncomment in case you want both strips wired in parallel at the base, sharing the same data pin

#define NUM_LEDS 144    // number OF LEDS of one strip: parallel: LEDS in strip going up
                        //                              series:   LEDS going up := LEDS going down = TOTAL number of LEDS/2
#define TIPMELT_LEDS 10 // number of LEDS of asingle strip contribuiting to the tip-melting effect

// Rename to avoid FastLED conflict:
#define SABER_BRIGHTNESS 100   // was: #define BRIGHTNESS 100

// Calibration values can be found by running the MPU6050_Calibration file in the tools folder of FX - SaberOS
#define X_ACCEL_OFFSET -6794
#define Y_ACCEL_OFFSET -1362
#define Z_ACCEL_OFFSET 410
#define X_GYRO_OFFSET 218
#define Y_GYRO_OFFSET -22
#define Z_GYRO_OFFSET 54

#define BATTERY_PIN 34  // Add BATTERY_PIN — pick a free ADC1 pin (ADC2 conflicts with WiFi)

/*
The battery's voltage can be to high for the esp to survive...
Therefore a voltage devider is needed.
Such a divider looks like this:

BAT +
|
R1
|
|-----------> ESP32 ADC-pin
|
R2
|
BAT -

The voltage on the ESP is calculated as: V_ESP = V_BAT * (R2/(R1 + R2))

A safe choice is to half the voltage -> R1 = R2. -> V_ESP = V_BAT * R/(2R) = V_BAT/2
As not to draw (to) much current, chose a big resistor, like 100k (100 000) ohms.
*/

#define R1 100000
#define R2 100000

// The program assumes the use a single 18650 cell, in case other layout/chemistry/... chnage the values in Battery.h

// 2. Add task config constants
#define BATTERY_HZ              1    // read once per second — no need to poll faster
#define BATTERY_TASK_STACK_SIZE 2048
#define BATTERY_TASK_PRIORITY   1

#define SWING_SENSITIVITY_INITIAL 1440
#define SWING_SENSITIVITY_MAX_THRESHOLD 20000
#define CLASH_THRESHOLD 33 // 1mg/LSB  milli G, 1G being earths force
#define CLASH_DURATION 2

#define BLADE_IGNITION_MS 800
#define BLADE_RETRACTION_MS 800
#define CLASH_FX_DURATION 750
#define LOCKUP_FX_DURATION 4000
#define BLASTER_FX_DURATION 500
#define SWING_FX_DURATION 300
#define TIPMELT_FX_DURATION 4000

#define LEDS_HZ 60
#define BUTTONS_HZ 30
#define MPU_HZ 60
#define DFPLAYER_HZ 20

#define LED_TASK_STACK_SIZE 6144
#define BUTTONS_TASK_STACK_SIZE 2048
#define MPU_TASK_STACK_SIZE 6144
#define DFPLAYER_TASK_STACK_SIZE 4096

#define LED_TASK_PRIORITY 1 // 3
#define BUTTONS_TASK_PRIORITY 1 // 1
#define MPU_TASK_PRIORITY 1 // 4
#define DFPLAYER_TASK_PRIORITY 1 // 2


#define DISABLE_PRINT 0

// Define a macro for Serial.println that only works if DISABLE_PRINT is not set to 1
#if defined(DISABLE_PRINT) && DISABLE_PRINT == 1
#define DEBUG_PRINT(x)    // Empty definition, effectively disables printing
#define DEBUG_PRINTLN(x)  // Empty definition, effectively disables printing
#else
#define DEBUG_PRINT(x) Serial.print(x)      // Enable Serial.println
#define DEBUG_PRINTLN(x) Serial.println(x)  // Enable Serial.println
#endif

#endif