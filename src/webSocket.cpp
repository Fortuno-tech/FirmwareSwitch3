#include "webSocket.h"
#include "led.h"
#include <ArduinoJson.h>
#include "webSocket.h"

WebSocketsServer webSocket = WebSocketsServer(81);
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(client_num);
            Serial.printf("[WS] Client %u connecté depuis %s\n", client_num, ip.toString().c_str());

            // Message de bienvenue
            DynamicJsonDocument doc(128);
            doc["status"] = "connected";
            String msg;
            serializeJson(doc, msg);
            webSocket.sendTXT(client_num, msg);
            break;
        }

        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client %u déconnecté\n", client_num);
            break;

        case WStype_TEXT: {
            String payloadStr = String((char*)payload);
            Serial.printf("[WS] Reçu: %s\n", payloadStr.c_str());

            DynamicJsonDocument doc(256);
            if (deserializeJson(doc, payloadStr) == DeserializationError::Ok) {
                String action = doc["action"];

                if (action == "toggle") {
                    String lampId = doc["output"];
                    bool state = (doc["state"] == "on");

                    setLampState(lampId, state);

                    // Réponse JSON
                    DynamicJsonDocument res(128);
                    res["output"] = lampId;
                    res["state"] = getLampState(lampId) ? "on" : "off";
                    String msg;
                    serializeJson(res, msg);
                    webSocket.sendTXT(client_num, msg);
                } 
                else if (action == "brightness") {
                    String lampId = doc["output"];
                    int value = doc["value"]; // valeur 0-100
                    setLampBrightness(lampId, value); // 🔹 PWM
                    // Réponse JSON
                    DynamicJsonDocument res(128);
                    res["output"] = lampId;
                    res["brightness"] = value;
                    String msg;
                    serializeJson(res, msg);
                    webSocket.sendTXT(client_num, msg);
                }
            }
            break;
        }
    }
}

void initWebSocket() {
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
    Serial.println("[WS] Serveur WebSocket démarré sur le port 81");
}

void handleWebSocket() {
    webSocket.loop();
}
