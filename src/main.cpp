#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include "led.h"
#include <ArduinoJson.h>
// Déclaration de la structure (à placer en haut du fichier)
typedef struct
{
  char ssid[32];
  char password[64];
  uint8_t checksum;
} WifiConfig;
// Configuration
#define AP_PASSWORD "Fortico1234"
const char *OTA_PASSWORD = "fortico1234";

// Variables
bool isAPMode = true;
unsigned long startTime = 0;

// Serveurs
ESP8266WebServer server(80);
DNSServer dnsServer;

// Prototypes
void setupWiFiAP();
void setupWebServer();
void saveWiFiConfig(String ssid, String password);

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  startTime = millis();

  // Initialisation LEDs
  pinMode(PIN_LED_D4, OUTPUT);
  pinMode(PIN_LED_D3, OUTPUT);
  digitalWrite(PIN_LED_D4, LOW);
  digitalWrite(PIN_LED_D3, HIGH);

  // Lecture et vérification EEPROM
  WifiConfig config;
  EEPROM.get(0, config);

  // Vérification checksum
  uint8_t sum = 0;
  for (size_t i = 0; i < strlen(config.ssid); i++)
    sum ^= config.ssid[i];
  for (size_t i = 0; i < strlen(config.password); i++)
    sum ^= config.password[i];

  if (config.ssid[0] != '\0' && sum == config.checksum)
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid, config.password);

    // Feedback visuel pendant la connexion
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000)
    {
      digitalWrite(PIN_LED_D3, !digitalRead(PIN_LED_D3)); // Clignotement rapide
      delay(100);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      isAPMode = false;
      digitalWrite(PIN_LED_D3, HIGH); // LED fixe = connecté
      Serial.println("\nConnecté au WiFi configuré!");
    }
  }

  // Fallback en mode AP si nécessaire
  if (isAPMode)
    setupWiFiAP();

  // Initialisation OTA
  ArduinoOTA.begin();

  // Serveur Web
  setupWebServer();

  // DNS pour Captive Portal
  if (isAPMode)
  {
    dnsServer.start(53, "*", WiFi.softAPIP());
  }

  Serial.println("\nPrêt !");
  Serial.print("IP: ");
  Serial.println(isAPMode ? WiFi.softAPIP() : WiFi.localIP());
}

void loop()
{
  server.handleClient();

  if (isAPMode)
  {
    dnsServer.processNextRequest();
  }

  ArduinoOTA.handle();

  // Gestion LED statut
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > (isAPMode ? 500 : 1000))
  {
    digitalWrite(PIN_LED_D3, !digitalRead(PIN_LED_D3));
    lastBlink = millis();
  }

  // Vérification périodique connexion WiFi (mode STA)
  if (!isAPMode && WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Perte connexion WiFi!");
   // ESP.restart();
  }
}

void setupWiFiAP()
{
  // Génération SSID unique avec MAC
  String apSSID = "SmartSwitchC3-" + WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15, 17);

  if (WiFi.softAP(apSSID.c_str(), AP_PASSWORD))
  {
    Serial.print("Point d'accès créé: ");
    Serial.println(apSSID);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    // LED clignotante en mode AP
    digitalWrite(PIN_LED_D3, LOW);
  }
  else
  {
    Serial.println("Erreur création point d'accès!");
  }
}

void setupWebServer() {
    // Middleware CORS pour toutes les réponses
    auto setCORSHeaders = []() {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
    };

    // ---------------- Contrôle LED ----------------
    server.on("/control", HTTP_POST, [&]() {
        setCORSHeaders();
        if (server.hasArg("state")) {
            String state = server.arg("state");
            digitalWrite(PIN_LED_D4, (state == "on") ? HIGH : LOW);
            server.send(200, "application/json",
                        "{\"status\":\"success\",\"led\":\"" + state + "\",\"timestamp\":" + millis() + "}");
        } else {
            server.send(400, "application/json", "{\"error\":\"missing_parameter\"}");
        }
    });

    server.on("/control", HTTP_OPTIONS, [&]() {
        setCORSHeaders();
        server.send(204);
    });

    // ---------------- Configuration WiFi ----------------
    server.on("/save", HTTP_POST, [&]() {
        setCORSHeaders();

        if (!server.hasArg("plain")) {
            server.send(400, "application/json", "{\"error\":\"missing_body\"}");
            return;
        }

        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error) {
            server.send(400, "application/json", "{\"error\":\"invalid_json\"}");
            return;
        }

        String ssid = doc["ssid"].as<String>();
        String password = doc["password"].as<String>();

        if (ssid.length() == 0 || password.length() < 8) {
            server.send(400, "application/json", "{\"error\":\"invalid_credentials\"}");
            return;
        }

        // Sauvegarde dans EEPROM, sans redémarrage immédiat
        saveWiFiConfig(ssid, password, false);

        // Réponse OK
        server.send(200, "application/json", "{\"status\":\"success\",\"message\":\"Redémarrage dans 5s...\"}");

        // Redémarrage différé après 5 secondes pour laisser le fetch se terminer
        delay(500);
        ESP.restart();
    });

    // ---------------- Infos système ----------------
    server.on("/api/info", HTTP_GET, [&]() {
        setCORSHeaders();
        DynamicJsonDocument doc(256);

        doc["mode"] = isAPMode ? "AP" : "STA";
        doc["ip"] = isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        doc["mac"] = WiFi.macAddress();
        doc["uptime"] = millis() / 1000;
        doc["led"] = digitalRead(PIN_LED_D4);
        doc["free_heap"] = ESP.getFreeHeap();

        String json;
        serializeJson(doc, json);
        server.send(200, "application/json", json);
    });

    // ---------------- Test ping ----------------
    server.on("/ping", HTTP_GET, [&]() {
        setCORSHeaders();
        server.send(200, "text/plain", "pong");
    });

    server.on("/ping", HTTP_OPTIONS, [&]() {
        setCORSHeaders();
        server.send(204);
    });

    // ---------------- Captive Portal ----------------
    server.on("/generate_204", HTTP_GET, [&]() {
        server.sendHeader("Location", "http://192.168.4.1");
        server.send(302, "text/plain", "");
    });

    server.on("/fwlink", HTTP_GET, [&]() {
        server.sendHeader("Location", "http://192.168.4.1");
        server.send(302, "text/plain", "");
    });

    // ---------------- Gestion erreurs ----------------
    server.onNotFound([&]() {
        if (server.method() == HTTP_OPTIONS) {
            setCORSHeaders();
            server.send(204);
        } else {
            server.send(404, "text/plain", "404: Not Found");
        }
    });

    // Démarrage serveur
    server.begin();
    Serial.println("Serveur HTTP démarré");
}

void saveWiFiConfig(String ssid, String password, bool doRestart = false)
{
    EEPROM.begin(512);

    WifiConfig config;
    uint8_t sum = 0;
    for (size_t i = 0; i < ssid.length(); i++) sum ^= ssid[i];
    for (size_t i = 0; i < password.length(); i++) sum ^= password[i];

    strncpy(config.ssid, ssid.c_str(), 31); config.ssid[31] = '\0';
    strncpy(config.password, password.c_str(), 63); config.password[63] = '\0';
    config.checksum = sum;

    EEPROM.put(0, config);
    EEPROM.commit();

    Serial.println("Configuration sauvegardée.");
    if(doRestart) {
        Serial.println("Redémarrage...");
        delay(500); // laisse le temps d’envoyer la réponse
        ESP.restart();
    }
}
void saveWiFiConfig(String ssid, String password, bool restart = true) {
    EEPROM.begin(512);
    
    // validation...
    WifiConfig config;
    uint8_t sum = 0;
    for (size_t i = 0; i < ssid.length(); i++) sum ^= ssid[i];
    for (size_t i = 0; i < password.length(); i++) sum ^= password[i];
    
    strncpy(config.ssid, ssid.c_str(), 31);
    config.ssid[31] = '\0';
    strncpy(config.password, password.c_str(), 63);
    config.password[63] = '\0';
    config.checksum = sum;

    EEPROM.put(0, config);
    EEPROM.commit();

    Serial.println("Configuration sauvegardée.");
    
    if (restart) {
        Serial.println("Redémarrage...");
        delay(1000);
        ESP.restart();
    }
} // pas de redémarrage immédiat
