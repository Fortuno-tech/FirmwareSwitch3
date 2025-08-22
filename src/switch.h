#ifndef SWITCH_H
#define SWITCH_H

#include <Arduino.h>

// Broches interrupteurs
#define SWITCH_D1 5   // GPIO5
#define SWITCH_D2 4   // GPIO4
#define SWITCH_D5 14  // GPIO14

// Broches lampes (à adapter selon ton câblage)
#define LAMP1 12   // GPIO12
#define LAMP2 13   // GPIO13
#define LAMP3 15   // GPIO15

void initSwitches();
void handleSwitches();

#endif
