#ifndef LED_H
#define LED_H

#include <Arduino.h>

// Définitions des pins LED
#define LAMPE_LED_D6 12 // Pin for the built-in LED on the board, change if necessary
#define LAMPE_LED_D7 13 // Pin for the built-in LED on the board, change if necessary
#define LAMPE_LED_D8 15 //let blue teste
#define LED_VERT_D4 2 //Verte

// Déclarations des fonctions LED
void initLEDs();
void turnOnLED_D6();
void turnOffLED_D6();
void turnOnLED_D7();
void turnOffLED_D7();
void toggleLED_D8();
void toggleLED_D8();

#endif
