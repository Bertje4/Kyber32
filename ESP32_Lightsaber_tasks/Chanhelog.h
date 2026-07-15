/*
I. Buxfixes

changed one pin that was not allowed by FastLED


II. [CHANGE]	add #define to use only a single button

Claude: https://claude.ai/share/bc0e9d99-ae94-44ce-ab28-e47d19f8e115

III. [CHANGE]	add #define to not use a crystal LED in the blade

Claude: https://claude.ai/share/5ee616e6-fbd3-4f30-8667-37fa03eb4ef6

IV. [CHANGE]	Pins

[CHANGE]	#define RX_DFPLAYER: 16 -> 1
[CHANGE]	#define TX_DFPLAYER: 17 -> 3
[CHANGE]	#define MAIN_BUTTON: 32 -> 17

[CHANGE]	#define LED_OUTPUT: 5 -> 13

[CHANGE]	#define NUM_LEDS: 100 -> 144
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	
[CHANGE]	

[CHANGE]	

V. [CHANGE]	Website

changed look of the website + changed code base to upload the html file via LittleFS instead od hard copying the html code into SaberWeb.cpp

VI. [CHANGE]	Strip Hardware changes

wrote some changes to allow for the strips to be wired in series at the tip instead of parallel at the base.

Changes were optimised by Claude: https://claude.ai/share/00edb387-8575-4bdd-a994-8060ea127d69
*/