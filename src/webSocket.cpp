#include "webSocket.h"
#include "led.h"
#include <ArduinoJson.h>
#include "webSocket.h"

WebSocketsServer webSocket = WebSocketsServer(81);

void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(client_num);
            Serial.printf("[WS] Client %u connecté depuis %s\n", client_num, ip.toString().c_str());

            // Envoyer l’état actuel des lampes au client
            for (int i = 1; i <= 3; i++) {
                DynamicJsonDocument doc(128);
                doc["lamp"] = i;
                doc["state"] = getLampState(i);   // fonction à créer qui retourne true/false
                String msg;
                serializeJson(doc, msg);
                webSocket.sendTXT(client_num, msg);
            }
            break;
        }

        case WStype_DISCONNECTED:
            Serial.printf("[WS] Client %u déconnecté\n", client_num);
            break;

        case WStype_TEXT: {
            String payloadStr = String((char*)payload);
            Serial.printf("[WS] Message reçu: %s\n", payloadStr.c_str());

            DynamicJsonDocument doc(256);
            if (deserializeJson(doc, payloadStr) != DeserializationError::Ok) {
                Serial.println("[WS] Erreur JSON");
                return;
            }

            String action = doc["action"];

            if (action == "toggle") {
                int lamp = doc["lamp"];
                bool state = doc["state"];
                setLampState(lamp, state);   // fonction qui allume/éteint

                // renvoyer l'état à tous les clients
                DynamicJsonDocument res(128);
                res["lamp"] = lamp;
                res["state"] = state;
                String msg;
                serializeJson(res, msg);
                webSocket.broadcastTXT(msg);
            }

            else if (action == "brightness") {
                int lampId = doc["lamp"];
                int value = doc["value"];
                setLampBrightness(lampId, value);   // PWM

                // renvoyer la valeur à tous les clients
                DynamicJsonDocument res(128);
                res["lamp"] = lampId;
                res["brightness"] = value;
                String msg;
                serializeJson(res, msg);
                webSocket.broadcastTXT(msg);
            }

            break;
        }

        default:
            break;
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
