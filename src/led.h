#ifndef LED_H
#define LED_H

#include <Arduino.h>
#define D6 12 // Pin for switch D1
#define D7 13 // Pin for switch D2
#define D8 15 // Pin for switch D5 
void initLamps();


void setLampState(const String &lampId, bool state);
bool getLampState(const String &lampId);
// ⚡ Ajouter cette ligne
void setLampBrightness(const String &lampId, int value);

#endif
