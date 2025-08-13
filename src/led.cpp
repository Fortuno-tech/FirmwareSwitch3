#include "led.h"

// Initialisation des LEDs
void initLEDs() {
  pinMode(LAMPE_LED_D6, OUTPUT);
  pinMode(LAMPE_LED_D7, OUTPUT);
  pinMode(LAMPE_LED_D8, OUTPUT);
  
  // Éteindre les LEDs au démarrage
  digitalWrite(LAMPE_LED_D6, LOW);
  digitalWrite(LAMPE_LED_D7, LOW);
  digitalWrite(LAMPE_LED_D8, LOW);
}

// Allumer LED D4 (Point d'accès actif)
void turnOnLED_D6() {
  digitalWrite(LAMPE_LED_D6, HIGH);
}

// Éteindre LED D4
void turnOffLED_D6() {
  digitalWrite(LAMPE_LED_D6, LOW);
}

// Allumer LED D3 (Appareil connecté)
void turnOnLED_D7() {
  digitalWrite(LAMPE_LED_D7, HIGH);
}

// Éteindre LED D3
void turnOffLED_D7() {
  digitalWrite(LAMPE_LED_D7, LOW);
}
void turnOffLED_D8() {
  digitalWrite(LAMPE_LED_D8, LOW);
}
void turnOnLED_D8() {
  digitalWrite(LAMPE_LED_D8, HIGH);
}

// Basculer LED D4
void toggleLED_D6() {
  digitalWrite(LAMPE_LED_D6, !digitalRead(LAMPE_LED_D6));
}

// Basculer LED D3
void toggleLED_D7() {
  digitalWrite(LAMPE_LED_D7, !digitalRead(LAMPE_LED_D7));
}
void toggleLED_D8() {
  digitalWrite(LAMPE_LED_D8, !digitalRead(LAMPE_LED_D8));
}
