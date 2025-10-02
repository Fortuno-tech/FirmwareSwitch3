#ifndef LED_STA_H
#define LED_STA_H

#include <Arduino.h>

// --- Déclaration de la broche et variables globales ---
#define ledSTA D3  // Broche utilisée pour la LED (doit être définie ici pour tous les fichiers)

// Déclarations externes des variables utilisées dans d'autres .cpp
extern unsigned long previousMillisBlink;
extern const long blinkInterval;
extern bool ledStateBlink;

extern unsigned long fadeIntervalSTA;
extern int brightnessSTA;
extern int fadeAmountSTA;
extern unsigned long previousMillisLEDFondu;

// --- Fonctions disponibles ---
void setupBlinkLED();
void updateBlinkLED();
void notification_STA();

#endif
