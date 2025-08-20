#include "switch.h"
#include "led.h"   // pour contrôler les lampes

// État précédent des interrupteurs (pour détecter les changements)
bool lastState1 = LOW;
bool lastState2 = LOW;
bool lastState3 = LOW;

void initSwitches() {
    pinMode(SWITCH1_PIN, INPUT_PULLUP);
    pinMode(SWITCH2_PIN, INPUT_PULLUP);
    pinMode(SWITCH3_PIN, INPUT_PULLUP);
}

void handleSwitches() {
    bool currentState1 = digitalRead(SWITCH1_PIN);
    bool currentState2 = digitalRead(SWITCH2_PIN);
    bool currentState3 = digitalRead(SWITCH3_PIN);

    // Détection du changement d'état
    if (currentState1 != lastState1) {
        if (currentState1 == HIGH) {
            toggleLamp(1); // allume/éteint lampe 1
        }
        lastState1 = currentState1;
    }

    if (currentState2 != lastState2) {
        if (currentState2 == HIGH) {
            toggleLamp(2); // allume/éteint lampe 2
        }
        lastState2 = currentState2;
    }

    if (currentState3 != lastState3) {
        if (currentState3 == HIGH) {
            toggleLamp(3); // allume/éteint lampe 3
        }
        lastState3 = currentState3;
    }
}
