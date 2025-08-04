#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "led.h"

#define PIN_LED_D4 2 // Pin for the built-in LED on the board, change if necessary
#define PIN_LED_D3 0 // Pin for the built-in LED on the board, change if necessary
#define PIN_POUSSOIR_D5 4 // Pin for the built-in LED on the board, change if necessary

// Configuration du point d'accès
String ssid; // Nom du point d'accès sera généré dynamiquement
const char* password = "Fortico1234"; // Mot de passe
const int maxConnections = 4; // Nombre maximum de connexions

// Variables globales
int connectedDevices = 0;
unsigned long lastBlinkTime = 0;
const unsigned long blinkInterval = 500; // Intervalle de clignotement en ms

// Fonction pour générer le nom du point d'accès avec l'adresse MAC
String generateSSID() {
  String macAddress = WiFi.macAddress();
  String macPrefix = macAddress.substring(0, 4); // Prend les 4 premiers caractères
  //macPrefix.replace(":", ""); // Supprime les deux-points
  return "SmartSwitchC3-" + macPrefix;
}

void setup() {
  Serial.begin(115200);

  // Initialiser les LEDs
  initLEDs();
  
  // Générer le nom du point d'accès avec l'adresse MAC
  ssid = generateSSID();
  
  // Configurer le mode point d'accès
  WiFi.mode(WIFI_AP);
  
  // Créer le point d'accès
  if (WiFi.softAP(ssid.c_str(), password)) {
    Serial.println("Point d'accès créé avec succès!");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Mot de passe: ");
    Serial.println(password);
    Serial.print("Adresse IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("Adresse MAC: ");
    Serial.println(WiFi.macAddress());
    // Allumer LED D4 pour indiquer que le point d'accès est actif
    turnOnLED_D4();
  } else {
    Serial.println("Erreur lors de la création du point d'accès!");
  }
}

void loop() {
  // Vérifier le nombre d'appareils connectés
  int currentConnections = WiFi.softAPgetStationNum();
  
  // Si le nombre de connexions a changé
  if (currentConnections != connectedDevices) {
    connectedDevices = currentConnections;
    
    if (connectedDevices > 0) {
      // Au moins un appareil est connecté
      turnOffLED_D4(); // Éteindre LED D4
      turnOnLED_D3();  // Allumer LED D3
      Serial.print("Appareil connecté! Nombre total: ");
      Serial.println(connectedDevices);
    } else {
      // Aucun appareil connecté
      turnOnLED_D4();  // Allumer LED D4
      turnOffLED_D3(); // Éteindre LED D3
      Serial.println("Aucun appareil connecté");
    }
  }
  
  // Afficher les informations de connexion périodiquement
  static unsigned long lastInfoTime = 0;
  if (millis() - lastInfoTime > 10000) { // Toutes les 10 secondes
    Serial.print("Appareils connectés: ");
    Serial.println(connectedDevices);
    Serial.print("Adresse IP du point d'accès: ");
    Serial.println(WiFi.softAPIP());
    lastInfoTime = millis();
  }
  
  delay(100); // Petite pause pour éviter de surcharger le processeur
}
