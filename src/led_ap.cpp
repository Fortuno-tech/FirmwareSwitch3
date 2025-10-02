#include "led_ap.h"
#define ledAp D2

// millis pour le fondu
unsigned long fadeInterval = 30;
int brightness = 0;
int fadeAmount = 5;
unsigned long previousMillisLED = 0;

// millis pour master
unsigned long TempsReel_M;
unsigned long DernierTemps_M;
int state_M = 0;

// millis pour slave
unsigned long TempsReel_S;
unsigned long DernierTemps_S;
int state_S = 0;

void setupFadeLED()
{
  pinMode(ledAp, OUTPUT);
  digitalWrite(ledAp, LOW);
}

// témoin pour la notification
void notification_Ap()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillisLED >= fadeInterval)
  {
    previousMillisLED = currentMillis;
    brightness += fadeAmount;

    if (brightness <= 0 || brightness >= 255)
    {
      fadeAmount = -fadeAmount;
    }
    analogWrite(ledAp, brightness); // Appliquer uniquement sur ledSTA
  }
}

void modeMaster()
{
  TempsReel_M = millis();

  switch (state_M) {
    case 0: // LED ON (1er clignotement)
      if (TempsReel_M - DernierTemps_M >= 200) {
        digitalWrite(ledAp, HIGH);
        DernierTemps_M = TempsReel_M;
        state_M = 1;
      }
      break;

    case 1: // LED OFF
      if (TempsReel_M - DernierTemps_M >= 200) {
        digitalWrite(ledAp, LOW);
        DernierTemps_M = TempsReel_M;
        state_M = 2;
      }
      break;

    case 2: // LED ON (2ème clignotement)
      if (TempsReel_M - DernierTemps_M >= 200) {
        digitalWrite(ledAp, HIGH);
        DernierTemps_M = TempsReel_M;
        state_M = 3;
      }
      break;

    case 3: // LED OFF
      if (TempsReel_M - DernierTemps_M >= 200) {
        digitalWrite(ledAp, LOW);
        DernierTemps_M = TempsReel_M;
        state_M = 4;
      }
      break;

    case 4: // Pause de 2 secondes
      if (TempsReel_M - DernierTemps_M >= 2000) {
        DernierTemps_M = TempsReel_M;
        state_M = 0;
      }
      break;
  }
}

void modeSlave()
{
TempsReel_S = millis();

switch (state_S) {
  case 0: // LED ON (clignotement)
    if (TempsReel_S - DernierTemps_S >= 200) {
      digitalWrite(ledAp, HIGH); // Allume la LED
      DernierTemps_S = TempsReel_S;
      state_S = 1;
    }
    break;

  case 1: // LED OFF
    if (TempsReel_S - DernierTemps_S >= 200) {
      digitalWrite(ledAp, LOW); // Éteint la LED
      DernierTemps_S = TempsReel_S;
      state_S = 2;
    }
    break;

  case 2: // Pause de 2 secondes
    if (TempsReel_S - DernierTemps_S >= 2000) {
      DernierTemps_S = TempsReel_S;
      state_S = 0; // Recommencer le cycle
    }
    break;
}

}