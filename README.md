# Lightsaber based on ESP32 [Work In Progress!]

Kyber32 is a Platform IO project designed to control a NeoPixel lightsaber with custom electronics based on an ESP32.
The code offered is a fully functional an heavily customizable operating system for your lightsaber.
This software is build on PetervanderBurgt's ESP32 Lightsaber project https://github.com/PetervanderBurgt/ESP32_Lightsaber, that itself is based upon FX - SaberOS.

# Features 

- support for pixel led strips in the blade. Base lid sabers would also work as long as you get a pixel LED
- 18 different sound fonts
- flash on clash
- smoothswing
- blaster effects
- blade lock up
- control via website
- on or two buttons
- LEDs in the buttons to show the current status of the saber

# Changes made to ESP32_lightsaber

- added an option to use only one button instead of two, like you can do in FX - SaberOS
- added the option to not use a crystal inside the saber
- changed the look of the website to be cooler
- allowed for wiring the strips in series at the top instead of parallel at the bottom. This configuration allows the orientation of the LED at the top of the blade to be parallel to the blades cross-section, dramatically increasing brightness of the tip
- allows for non-RGB/single colour buttons
- monitors the battery as to not over drain and shut of when charging

# How to use?

1. download the Platform IO folder and save it on your computer
2. in VS Code, (if not already installed, install the Platform IO extension)
3. click the Platform IO logo and chose "open project" and open the folder you created in step 1.
4. IMPORTANT: open pinConig.h and change the preferences according to your setup! This step is very important to ensure proper functionality.
5. If interested, snoop around the code base and make changes for even more customization! (Most things you would want to change are already in pinConfig.h)
6. Upload your code and have fun!*

*The ESP32 S2 uses internal USB and can therefore be a bit fussy. To make sure you can upload your program you must first put the microcontroller into bootloader mode. To do this you hold down the bootbuttom while (re)connecting the ESP to the computer by either pressing the Reset button/shorting the EN pin to GND or just (unplugging and re) pluging (in) the ESP. Alternatively, the bootbutton just shorts GPIO 0 to GND, so if you (accidentaly) remove it, you can just short that pin to GND. 
