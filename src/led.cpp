#include "led.h"

// Initialisation des LEDs
void initLEDs() {
  pinMode(PIN_LED_D4, OUTPUT);
  pinMode(PIN_LED_D3, OUTPUT);
  
  // Éteindre les LEDs au démarrage
  digitalWrite(PIN_LED_D4, LOW);
  digitalWrite(PIN_LED_D3, LOW);
}

// Allumer LED D4 (Point d'accès actif)
void turnOnLED_D4() {
  digitalWrite(PIN_LED_D4, HIGH);
}

// Éteindre LED D4
void turnOffLED_D4() {
  digitalWrite(PIN_LED_D4, LOW);
}

// Allumer LED D3 (Appareil connecté)
void turnOnLED_D3() {
  digitalWrite(PIN_LED_D3, HIGH);
}

// Éteindre LED D3
void turnOffLED_D3() {
  digitalWrite(PIN_LED_D3, LOW);
}

// Basculer LED D4
void toggleLED_D4() {
  digitalWrite(PIN_LED_D4, !digitalRead(PIN_LED_D4));
}

// Basculer LED D3
void toggleLED_D3() {
  digitalWrite(PIN_LED_D3, !digitalRead(PIN_LED_D3));
}
