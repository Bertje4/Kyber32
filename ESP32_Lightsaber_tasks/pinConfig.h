#ifndef PINCONFIG_H
#define PINCONFIG_H

/*
This headerfile contains all the settings you need to change. Review every single one, as they are specific to your personal setup!
More advanced settings can be found in the specific headerfiles and at the bottom under "Miscellaneous", but those should not be tweaked in most cases, succes!
*/

// **** UART Communication ****
#define RX_DFPLAYER 1
#define TX_DFPLAYER 3

// **** Buttons ****
/*
When you only want to use a single button (fully functional but interface is a bit clumbsy (not really a problem as you can just use the website)):
leave line 21 uncommented and don't worry about every thing with SECOND_ as these definitions will not be used, just fill in MAIN_.

If you do want to use a second button, comment line 21 and make sure both MAIN_ and SECOND_ settings are correct
*/

#define SINGLE_BUTTON_MODE  // comment this out to re-enable two-button mode

#define MAIN_BUTTON 17

/*
If buttons aren't RGB, no problem, just connect the corresponding colour and do NOT use the other two pins you specify here.
The program defines these pins as outputs, so they can't be used for other tasks.

When you only want to use one button, leave 
*/

// CHANGEME - Button Light wiring
#define MAIN_RED 23
#define MAIN_GREEN 19
#define MAIN_BLUE 18

#ifdef SINGLE_BUTTON_MODE
  #define LONG_PRESS_LOCKUP_MS 1000  // hold longer than this = lockup, shorter = retract
#endif

#ifndef SINGLE_BUTTON_MODE  // When using two buttons -> 21 uncommented: make sure these pins are defined correctly
  #define SECOND_BUTTON 25
  #define SECOND_RED 13
  #define SECOND_GREEN 33
  #define SECOND_BLUE 26
#endif

// **** In-hilt-Kyber Crystal ****
// #define CRYSTAL   // uncomment to enable crystal chamber LED

#ifdef CRYSTAL  // change these values when using a crystal
  #define CRYSTAL_LED_OUTPUT 15
  #define NUM_LEDS_CRYSTAL 1
#else           // leave as is!
  #define NUM_LEDS_CRYSTAL 0   // zero-length, never written to
#endif

// **** NeoPixel Blade ****
/*
The original library expects both LED strips (glued back to back for a fuller look) to be wired in parallel in the hilt (Figure 1).
This version also allows to wire the strips in series at the to of the blade -> arrows of the strip with connector go up the blade, where that
strip is soldered to the other one with the arrows facing the hilt, in effect, creating one U-shaped strip going up and down the blade.
This configuration has the advantages that the code can individually adress all the LEDS (instead of both strips doing accictily the same), and
it allows for putting the LED at the top of te blade parallel to a cross-section of the blade, allowing for more light in the tip (FIGURE 2).

It is however important that -- when using the series configuration -- the total number of LEDS remains even AND equal op and down.
The code uses the horizontal LED as the first LED of the strip going down the blade.
So you have NUM_LEDS starting with the LED with the connector until, including, the LED BEFORE the horizintal one.
And NUM_LEDS starting (& including) the horizontal LED going back to the hilt.

Graphical representation

                  Hilt                                              Tip of blade

---> : LED strip with direction facing >
...> : electrical connection

Figure 1: parallel

          |...>   -----------------------------------------------------> strip one  |
ESP ......|                                                                         |--> sharing the same data line at the bottom -> both strips act in accactely the same way
          |...>   -----------------------------------------------------> strip two  | 

Figure 2: series

ESP .........>    -----------------------------------------------------  "strip one"      |
                                                                      |  "horizontal LED" |--> forming one big strip going up and down the blade, with ONLY strip one being (directly) connected to the esp
                  <----------------------------------------------------  "strip two"      |
*/

#define STRIPS_IN_SERIES // uncomment in case you want both strips wired in parallel at the base, sharing the same data pin

#define NUM_LEDS 144    // number OF LEDS of one strip: parallel: LEDS in strip going up
                        //                              series:   LEDS going up := LEDS going down = TOTAL number of LEDS/2

#define TIPMELT_LEDS 10 // number of LEDS of a single strip contribuiting to the tip-melting effect

#define SABER_BRIGHTNESS 100  // percentage

// **** MPU-6050 Accelerometer & Gyroscope ****
// Pins used for I2C communication
#define SDA 21 
#define SCL 22

// Interrupt: this pin must be connected so the saber can immedeatly handle Flash On Clash
#define MPU_INTERRUPT 4

/*
The MPU-6050 chips do not come precalibrated so there measurements do not correspond (perfectly) to reality.
Therefore this chip MUST be calibrated before use!

The calibration values can be found by running the MPU6050_Calibration file in the tools folder on Github.
Make sure you select the CORRECT orientation of the MPU-6050 unit.

After verifying the code and uploading the program, the serial monitor will print the necesarry ofsets. Copy those values and fill them in here.

Succes!
*/

#define X_ACCEL_OFFSET 0.079970
#define Y_ACCEL_OFFSET 0.545818
#define Z_ACCEL_OFFSET 2.828468
#define X_GYRO_OFFSET -0.002267
#define Y_GYRO_OFFSET 0.021320
#define Z_GYRO_OFFSET 0.012824

// **** Battery ****
/*
To protect the battery from draining to low (and also for eas of use), the program must have a way of measuring how much juice is still left in the battery.
To do this, we can measure the battery's voltage as that diminisches when the battery drains.

BUT, the battery's voltage can be to high for the esp to survive...
Therefore a voltage devider is NEEDED.
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

Side note: the program assumes the use of a single 18650 cell, in case of other layout/chemistry/... change the values in Battery.h
*/

#define R1 100000
#define R2 100000

#define BATTERY_PIN 34  // Pick a free ADC1 pin (ADC2 conflicts with WiFi)

// **** Miscellaneous: can be tweaked to preference but require knowledge of the software ****
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