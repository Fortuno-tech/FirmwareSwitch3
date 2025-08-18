#include "led.h"

// Initialisation des LEDs
void initLEDs() {
  // Configurer les pins des LEDs
  pinMode(LAMPE_LED_D6, OUTPUT);
  pinMode(LAMPE_LED_D7, OUTPUT);
  pinMode(LAMPE_LED_D8, OUTPUT);

  pinMode(OUTPUT_LAMPE_D1, OUTPUT);
  pinMode(OUTPUT_LAMPE_D2, OUTPUT);
  pinMode(OUTPUT_LAMPE_D3, OUTPUT);

  // Éteindre les LEDs au démarrage
  digitalWrite(LAMPE_LED_D6, LOW);
  digitalWrite(LAMPE_LED_D7, LOW);
  digitalWrite(LAMPE_LED_D8, LOW);
  // Éteindre les sorties des lampes
  digitalWrite(OUTPUT_LAMPE_D1, LOW);
  digitalWrite(OUTPUT_LAMPE_D2, LOW);
  digitalWrite(OUTPUT_LAMPE_D3, LOW);
}
// Allumer LED D4 (Point d'accès actif)
void turnOnLED_D6() {
  digitalWrite(LAMPE_LED_D6, HIGH);
  digitalWrite(OUTPUT_LAMPE_D1, HIGH);


}

// Éteindre LED D4
void turnOffLED_D6() {
  digitalWrite(LAMPE_LED_D6, LOW);
  digitalWrite(OUTPUT_LAMPE_D1, LOW);
}

// Allumer LED D3 (Appareil connecté)
void turnOnLED_D7() {
  digitalWrite(LAMPE_LED_D7, HIGH);
  digitalWrite(OUTPUT_LAMPE_D2, HIGH);
}

// Éteindre LED D3
void turnOffLED_D7() {
  digitalWrite(LAMPE_LED_D7, LOW);
  digitalWrite(OUTPUT_LAMPE_D2, LOW);
}
void turnOffLED_D8() {
  digitalWrite(LAMPE_LED_D8, LOW);
  digitalWrite(OUTPUT_LAMPE_D3, LOW);
}
void turnOnLED_D8() {
  digitalWrite(LAMPE_LED_D8, HIGH);
  digitalWrite(OUTPUT_LAMPE_D3, HIGH);
}

// Basculer LED D4
void toggleLED_D6() {
  digitalWrite(LAMPE_LED_D6, !digitalRead(LAMPE_LED_D6));
  digitalWrite(OUTPUT_LAMPE_D1, !digitalRead(OUTPUT_LAMPE_D1));
}

// Basculer LED D3
void toggleLED_D7() {
  digitalWrite(LAMPE_LED_D7, !digitalRead(LAMPE_LED_D7));
  digitalWrite(OUTPUT_LAMPE_D2, !digitalRead(OUTPUT_LAMPE_D2));
}
void toggleLED_D8() {
  digitalWrite(LAMPE_LED_D8, !digitalRead(LAMPE_LED_D8));
  digitalWrite(OUTPUT_LAMPE_D3, !digitalRead(OUTPUT_LAMPE_D3));
}