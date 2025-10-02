#ifndef WEBSOCKET_CONF_H
#define WEBSOCKET_CONF_H

#include <Arduino.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
extern String ssidAp;
extern String passwordAp;
extern AsyncWebServer server;
extern String deviceRole;
extern String value;
extern String ssid ;
extern String password ;
bool initLittleFS();
void formatage();
void setupInput();
void saveWiFiConfig(const char *ssid, const char *password);
bool loadWiFiConfig();
void saveToSPIFFS(const String &filename, const String &data);
String loadFromSPIFFS(const String &filename);
void configureEspNow();
void deficeRole();
void OnDataRecvS(uint8_t *mac, uint8_t *data, uint8_t len);
void OnDataRecvM(uint8_t *mac, uint8_t *data, uint8_t len);
void removeSlaveByMac(uint8_t *macToRemove);
// void connectWiFi();
bool loadWiFiConfigAp();
// void handleWiFi();
void loadAllSlavesFromSPIFFS();
bool loadMacFromSPIFFS(uint8_t *mac);
void saveAllSlavesToSPIFFS();
void notifyClients();
bool isMacInList(uint8_t *mac);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void initWebSocket();
void Server();
void startAPMode();
#endif