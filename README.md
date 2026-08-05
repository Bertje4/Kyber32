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


