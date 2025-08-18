#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "led.h"
#include "switch.h"
#include <ArduinoJson.h>

#define AP_PASSWORD "Fortico1234"
const char *OTA_PASSWORD = "fortico1234";

ESP8266WebServer server(80);
unsigned long startTime;

#define EEPROM_SIZE 3  // 0 = LED state, 1 = brightness

#define SWITCH_D1 5 // pine sortie de ESP  poue ALLER au entre de interrupteur
#define SWITCH_D2 4 // pine sortie de ESP poue ALLER au entre de interrupteur
#define SWITCH_D5 14 //pine sortie de ESP poue ALLER au entre de interrupteur

void setupWiFiAP();
void setupWebServer();


void setup() {
  Serial.begin(115200);
  startTime = millis();

  pinMode(LAMPE_LED_D6, OUTPUT);
  pinMode(LAMPE_LED_D7, OUTPUT);
  pinMode(LAMPE_LED_D8, OUTPUT);

  digitalWrite(LAMPE_LED_D6, LOW);
  digitalWrite(LAMPE_LED_D7, LOW);
  digitalWrite(LAMPE_LED_D8, LOW);

  analogWriteRange(1023);
  analogWriteFreq(1000);

  // --- EEPROM pour persistance ---
  EEPROM.begin(EEPROM_SIZE);
  uint8_t ledState = EEPROM.read(0);
  uint8_t brightness = EEPROM.read(1);

  digitalWrite(LAMPE_LED_D7, ledState ? HIGH : LOW);
  analogWrite(LAMPE_LED_D7, map(brightness, 0, 100, 0, 1023));

  setupWiFiAP();
  ArduinoOTA.begin();
  setupWebServer();

  Serial.println("\nPrêt !");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  server.handleClient();
  ArduinoOTA.handle();
}


void setupWiFiAP() {
  String apSSID = "SmartSwitchC3-" + WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15, 17);
  if (WiFi.softAP(apSSID.c_str(), AP_PASSWORD)) {
    Serial.print("Point d'accès créé: ");
    Serial.println(apSSID);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Erreur création point d'accès!");
  }
} 

void setupWebServer() {
  auto setCORSHeaders = []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  };

  // --- Contrôle LED ---
// --- Contrôle LED ---
server.on("/control", HTTP_POST, [&]() {
  setCORSHeaders();

  if (server.hasArg("state") && server.hasArg("output")) {
    String state = server.arg("state");
    String output = server.arg("output");

    if (state == "on") {
      if (output == "LAMPE_LED_D6") {
        turnOnLED_D6();
        EEPROM.write(0, 1);
      } else if (output == "LAMPE_LED_D7") {
        turnOnLED_D7();
        EEPROM.write(1, 1);
      } else if (output == "LAMPE_LED_D8") {
        turnOnLED_D8();
        EEPROM.write(2, 1);
      }
    } else { // OFF
      if (output == "LAMPE_LED_D6") {
        turnOffLED_D6();
        EEPROM.write(0, 0);
      } else if (output == "LAMPE_LED_D7") {
        turnOffLED_D7();
        EEPROM.write(1, 0);
      } else if (output == "LAMPE_LED_D8") {
        turnOffLED_D8();
        EEPROM.write(2, 0);
      }
    }
    EEPROM.commit();

    // Réponse JSON avec LED et état
    server.send(200, "application/json",
      "{\"status\":\"success\",\"output\":\"" + output + "\",\"led\":\"" + state + "\",\"timestamp\":" + String(millis()) + "}"
    );

  } else {
    server.send(400, "application/json", "{\"error\":\"missing_parameter\"}");
  }
});

  server.on("/brightness", HTTP_POST, [&]() {
    setCORSHeaders();
    if (server.hasArg("value") && server.hasArg("output")) {
      int brightness = server.arg("value").toInt();
      String output = server.arg("output");
      brightness = constrain(brightness, 0, 100);
        if (output == "LAMPE_LED_D6") {
      analogWrite(LAMPE_LED_D6, map(brightness, 0, 100, 0, 1023));
      } else if (output == "LAMPE_LED_D7") {
      analogWrite(LAMPE_LED_D7, map(brightness, 0, 100, 0, 1023));
      EEPROM.write(0, brightness > 0 ? 1 : 0);
      } else if (output == "LAMPE_LED_D8") {
      analogWrite(LAMPE_LED_D8, map(brightness, 0, 100, 0, 1023));
      EEPROM.write(0, brightness > 0 ? 1 : 0);
      }
      // LED automatiquement ON si brightness > 0
      EEPROM.commit();
      server.send(200, "application/json",
                  "{\"status\":\"ok\",\"brightness\":" + String(brightness) + ",\"led\":\"" + (brightness>0?"on":"off") + "\"}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing_value\"}");
    }
  });

  server.on("/control", HTTP_OPTIONS, [&]() {
    setCORSHeaders();
    server.send(204);
  });

  // --- Informations système ---
  server.on("/api/info", HTTP_GET, [&]() {
    setCORSHeaders();
    DynamicJsonDocument doc(256);
    doc["ip"] = WiFi.softAPIP().toString();
    doc["mac"] = WiFi.macAddress(); 
    doc["uptime"] = millis() / 1000;
    uint8_t ledState = EEPROM.read(0);
    uint8_t brightness = EEPROM.read(1);
    doc["led_state"] = ledState ? "on" : "off";
    doc["brightness"] = brightness;
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/ping", HTTP_GET, [&]() {
    setCORSHeaders();
    server.send(200, "text/plain", "pong");
  });

  server.onNotFound([&]() {
    if (server.method() == HTTP_OPTIONS) {
      setCORSHeaders();
      server.send(204);
    } else {
      server.send(404, "text/plain", "404: Not Found");
    }
  });

  server.begin();
  Serial.println("Serveur HTTP démarré");
}
