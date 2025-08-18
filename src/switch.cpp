#include "switch.h"
#include "led.h"
#include <Arduino.h>

// Variables pour mémoriser l’état des LEDs
bool lampeD6State = false;
bool lampeD7State = false;
bool lampeD8State = false;

// Variables pour mémoriser l’ancien état des interrupteurs
int lastD1 = HIGH;
int lastD2 = HIGH;
int lastD5 = HIGH;

void initSwitch() {
    pinMode(SWITCH_D1, INPUT_PULLUP);
    pinMode(SWITCH_D2, INPUT_PULLUP);
    pinMode(SWITCH_D5, INPUT_PULLUP);

    // Initialisation LEDs éteintes
    digitalWrite(LAMPE_LED_D6, LOW);
    digitalWrite(LAMPE_LED_D7, LOW);
    digitalWrite(LAMPE_LED_D8, LOW);
}

void outputPin() {
    int etatD1 = digitalRead(SWITCH_D1);
    int etatD2 = digitalRead(SWITCH_D2);
    int etatD5 = digitalRead(SWITCH_D5);

    // Détection d’un changement (front descendant : HIGH -> LOW)
    if (etatD1 == LOW && lastD1 == HIGH) {
        lampeD6State = !lampeD6State; // On bascule l’état
        digitalWrite(LAMPE_LED_D6, lampeD6State ? HIGH : LOW);
    }
    if (etatD2 == LOW && lastD2 == HIGH) {
        lampeD7State = !lampeD7State;
        digitalWrite(LAMPE_LED_D7, lampeD7State ? HIGH : LOW);
    }
    if (etatD5 == LOW && lastD5 == HIGH) {
        lampeD8State = !lampeD8State;
        digitalWrite(LAMPE_LED_D8, lampeD8State ? HIGH : LOW);
    }

    // Mise à jour des anciens états
    lastD1 = etatD1;
    lastD2 = etatD2;
    lastD5 = etatD5;
}
