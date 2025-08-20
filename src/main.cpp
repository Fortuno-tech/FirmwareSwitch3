#include <Arduino.h>
#include "wifiManager.h"
#include "led.h"
#include "switch.h"
#include "webSocket.h"

void setup() {
  Serial.begin(115200);

  initWiFiAP();     // ⚡ Crée le réseau WiFi "SmartSwitchXXXX"
  initLamps();      // ⚡ Init GPIO lampes
  initWebSocket();  // ⚡ Démarre serveur WebSocket
}

void loop() {
   handleSwitches();
  handleWebSocket();
}
// ⚡ Boucle principale pour gérer les connexions WebSocket