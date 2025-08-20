#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>

// Définition des broches des interrupteurs
#define SWITCH1_PIN 5
#define SWITCH2_PIN 12
#define SWITCH3_PIN 14

// Prototypes
void initSwitches();
void handleSwitches();

#endif
