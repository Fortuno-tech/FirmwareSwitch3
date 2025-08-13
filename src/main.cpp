#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "led.h"
#include <ArduinoJson.h>

#define AP_PASSWORD "Fortico1234"
const char *OTA_PASSWORD = "fortico1234";

ESP8266WebServer server(80);
unsigned long startTime;
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
  analogWriteRange(1023); // Valeurs 0-1023
  analogWriteFreq(1000);  // Fréquence PWM

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

  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500) {
   // digitalWrite(PIN_LED_D3, !digitalRead(PIN_LED_D3));
    lastBlink = millis();
  }
}

void setupWiFiAP() {
  String apSSID = "SmartSwitchC3-" + WiFi.macAddress().substring(12, 14) + WiFi.macAddress().substring(15, 17);
  if (WiFi.softAP(apSSID.c_str(), AP_PASSWORD)) {
    Serial.print("Point d'accès créé: ");
    Serial.println(apSSID);
    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());
    digitalWrite(LED_VERT_D4, HIGH);
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

  // ---- Contrôle LED ----
 
server.on("/control", HTTP_POST, [&]() {
    setCORSHeaders();
    if (server.hasArg("state")) {
      String state = server.arg("state");
      if(state == "on"){
        turnOnLED_D7();
      }else{
        turnOffLED_D7();
      }
      server.send(200, "application/json",
                "{\"status\":\"success\",\"led\":\"" + state + "\",\"timestamp\":" + millis() + "}");
    } else {
      server.send(400, "application/json", "{\"error\":\"missing_parameter\"}");
    }
  });
  server.on("/brightness", HTTP_POST, [&]() {
    setCORSHeaders();
    if (server.hasArg("value")) {
        int brightness = server.arg("value").toInt();
           brightness = constrain(brightness, 0, 100);
          analogWrite(LAMPE_LED_D7, map(brightness, 0, 100, 0, 1023));
           server.send(200, "application/json",
            "{\"status\":\"ok\",\"led\":\"on\",\"brightness\":" + String(brightness) + "}");
  
       
    } else {
        server.send(400, "application/json", "{\"error\":\"missing_value\"}");
    }
});

  server.on("/control", HTTP_OPTIONS, [&]() {
    setCORSHeaders();
    server.send(204);
  });

  // ---- Informations système ----
  server.on("/api/info", HTTP_GET, [&]() {
    setCORSHeaders();
    DynamicJsonDocument doc(256);
    doc["ip"] = WiFi.softAPIP().toString();
    doc["mac"] = WiFi.macAddress();
    doc["uptime"] = millis() / 1000;
    doc["led"] = digitalRead(LAMPE_LED_D6);

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