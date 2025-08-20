#include "led.h"

// Variables pour garder l'état
bool lampD6 = false;
bool lampD7 = false;
bool lampD8 = false;

void initLamps() {
    pinMode(D6, OUTPUT);
    pinMode(D7, OUTPUT);
    pinMode(D8, OUTPUT);

    digitalWrite(D6, LOW);
    digitalWrite(D7, LOW);
    digitalWrite(D8, LOW);
}

void setLampState(const String &lampId, bool state) {
    if (lampId == "LAMPE_LED_D6") {
        lampD6 = state;
        digitalWrite(D6, state ? HIGH : LOW);
    } else if (lampId == "LAMPE_LED_D7") {
        lampD7 = state;
        digitalWrite(D7, state ? HIGH : LOW);
    } else if (lampId == "LAMPE_LED_D8") {
        lampD8 = state;
        digitalWrite(D8, state ? HIGH : LOW);
    }
}

void setLampBrightness(const String &lampId, int value) {
    // Convertir en PWM (0-1023 pour ESP8266)
    int pwm = map(value, 0, 100, 0, 1023);

    if (lampId == "LAMPE_LED_D6") {
        analogWrite(D6, pwm);
    } else if (lampId == "LAMPE_LED_D7") {
        analogWrite(D7, pwm);
    } else if (lampId == "LAMPE_LED_D8") {
        analogWrite(D8, pwm);
    }
}
bool getLampState(const String &lampId) {
    if (lampId == "LAMPE_LED_D6") return lampD6;
    if (lampId == "LAMPE_LED_D7") return lampD7;
    if (lampId == "LAMPE_LED_D8") return lampD8;
    return false;
}
