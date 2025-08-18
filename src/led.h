#ifndef LED_H
#define LED_H

#include <Arduino.h>

// Définitions des pins LED
#define LAMPE_LED_D6 12 // Pin for the built-in LED on the board, change if necessary
#define LAMPE_LED_D7 13 // Pin for the built-in LED on the board, change if necessary
#define LAMPE_LED_D8 15 //let blue teste
#define LED_VERT_D4 2 //Verte

#define OUTPUT_LAMPE_D1 5 // Pin for the first output lamp
#define OUTPUT_LAMPE_D2 4 // Pin for the second output lamp
#define OUTPUT_LAMPE_D3 0 // Pin for the third output lamp



// Déclarations des fonctions LED
void initLEDs();
void turnOnLED_D6();
void turnOffLED_D6();
void turnOnLED_D7();
void turnOffLED_D7();
void turnOffLED_D8();
void turnOnLED_D8();

#endif
