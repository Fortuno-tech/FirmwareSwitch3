#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <Arduino.h>
#include <ESP8266WiFi.h>   // ESP32 → remplacer par <WiFi.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

void initWebSocket();
void handleWebSocket();

extern WebSocketsServer webSocket;

#endif
