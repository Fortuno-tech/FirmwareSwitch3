#include "led_sta.h"

#define ledSTA D3  // Définir la broche de la LED
unsigned long previousMillisBlink = 0;
const long blinkInterval = 500;  // Intervalle de clignotement en millisecondes
bool ledStateBlink = false;

// millis pour le fondu
unsigned long fadeIntervalSTA = 30;
int brightnessSTA = 0;
int fadeAmountSTA = 5;
unsigned long previousMillisLEDFondu = 0;

void setupBlinkLED() {
    pinMode(ledSTA, OUTPUT);
    digitalWrite(ledSTA, LOW);
}

void updateBlinkLED() {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillisBlink >= blinkInterval) {
        previousMillisBlink = currentMillis;
        ledStateBlink = !ledStateBlink;
        digitalWrite(ledSTA, ledStateBlink);
    }
}

// témoin pour la notification
void notification_STA () {
  unsigned long currentMillisFondu = millis();
  if (currentMillisFondu - previousMillisLEDFondu >= fadeIntervalSTA) {
    previousMillisLEDFondu = currentMillisFondu;
    brightnessSTA += fadeAmountSTA;

    if (brightnessSTA <= 0 || brightnessSTA >= 255) {
      fadeAmountSTA = -fadeAmountSTA;
    }
    analogWrite(ledSTA, brightnessSTA);  // Appliquer uniquement sur ledSTA
  }
}
