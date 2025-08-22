#include "switch.h"

// Variables pour mémoriser l’état précédent des interrupteurs
bool lastState1 = HIGH;
bool lastState2 = HIGH;
bool lastState3 = HIGH;

void initSwitches() {
    pinMode(SWITCH_D1, INPUT_PULLUP);
    pinMode(SWITCH_D2, INPUT_PULLUP);
    pinMode(SWITCH_D5, INPUT_PULLUP);

    pinMode(LAMP1, OUTPUT);
    pinMode(LAMP2, OUTPUT);
    pinMode(LAMP3, OUTPUT);

    // Éteindre au démarrage
    digitalWrite(LAMP1, LOW);
    digitalWrite(LAMP2, LOW);
    digitalWrite(LAMP3, LOW);
}

void handleSwitches() {
    bool current1 = digitalRead(SWITCH_D1);
    bool current2 = digitalRead(SWITCH_D2);
    bool current3 = digitalRead(SWITCH_D5);

    // Détection changement interrupteur 1
    if (current1 != lastState1 && current1 == LOW) {
        digitalWrite(LAMP1, !digitalRead(LAMP1)); // Toggle lampe 1
    }
    lastState1 = current1;

    // Détection changement interrupteur 2
    if (current2 != lastState2 && current2 == LOW) {
        digitalWrite(LAMP2, !digitalRead(LAMP2)); // Toggle lampe 2
    }
    lastState2 = current2;

    // Détection changement interrupteur 3
    if (current3 != lastState3 && current3 == LOW) {
        digitalWrite(LAMP3, !digitalRead(LAMP3)); // Toggle lampe 3
    }
    lastState3 = current3;
}
