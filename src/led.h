#ifndef LED_H
#define LED_H

#include <Arduino.h>

// Définitions des pins LED
#define PIN_LED_D4 2 // Pin for the built-in LED on the board, change if necessary
#define PIN_LED_D3 0 // Pin for the built-in LED on the board, change if necessary

// Déclarations des fonctions LED
void initLEDs();
void turnOnLED_D4();
void turnOffLED_D4();
void turnOnLED_D3();
void turnOffLED_D3();
void toggleLED_D4();
void toggleLED_D3();

#endif
