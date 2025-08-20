#include "wifiManager.h"

const char* apPassword = "Fortico1234";
String apSSID = "";

String getModuleSSID() {
    return apSSID;
}

void initWiFiAP() {
    // Récupérer l'adresse MAC
    String mac = WiFi.macAddress();   // exemple : "84:F3:EB:12:34:56"
    mac.replace(":", "");             // enlever les ":"
    apSSID = "SmartSwitch" + mac.substring(0, 4); // SmartSwitch84F3

    // Démarrer en mode Point d'accès
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str(), apPassword);

    Serial.println("\n=== Point d'accès WiFi créé ===");
    Serial.print("SSID : ");
    Serial.println(apSSID);
    Serial.print("Password : ");
    Serial.println(apPassword);
    Serial.print("IP AP : ");
    Serial.println(WiFi.softAPIP());
}
