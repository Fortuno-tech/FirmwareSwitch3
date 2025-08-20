#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>   // ou <WiFi.h> si ESP32

void initWiFiAP();         // Démarrer en point d'accès
String getModuleSSID();    // Récupérer le SSID généré

#endif
