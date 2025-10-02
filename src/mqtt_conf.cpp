// PARTY MQTT

#include "websocketConf.h"
#include <ArduinoJson.h>
#include "mqtt_conf.h"
#include <PubSubClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266WiFi.h>
#include "led_sta.h"
#include "led_ap.h"
#include "HorlogeModule.h"
#include "mode_conf.h"
#include "config.h"

bool state1 = 0;
bool state2 = 0;
bool state3 = 0;
int outputPin1 = D6;
int outputPin2 = D7;
int outputPin3 = D8;
#define inputPin1 D1
#define inputPin3 D2
#define inputPin0 D5

extern HorlogeModule horloge;
// ---- CONFIGURATION HIVE MQ ----
const char *mqtt_server = "e59af3ed375b42f6ad6c44f423c06a66.s1.eu.hivemq.cloud"; // Remplacez par l'URL de votre instance
const int mqtt_port = 8883;                                                      // Utilisation de MQTT sécurisé (TLS)
const char *mqtt_user = "smart_l1";
const char *mqtt_password = "Fortico1234";

// 🔹 Définition du client MQTT sécurisé
BearSSL::WiFiClientSecure espClient;
PubSubClient client(espClient);
bool wifiConnected = false;

// ID et topic MQTT dynamiques basés sur l'adresse MAC
String macAddress = WiFi.macAddress();
String mqttClientId = "Model C1" + macAddress;
String mqtt_topic = "smartSwitch C1/" + macAddress + "/status"; // Topic unique par ESP

bool lws_status1;
bool old_lws_status1;
bool lws_status2;
bool old_lws_status2;
bool lws_status3;
bool old_lws_status3;
static bool scanningWiFi = false;

void checkForUpdates(const char *firmware_url);

// Variables globales
bool tryingWiFi = false;
unsigned long wifiDisconnectTime = 0;
const unsigned long wifiReconnectTimeout = 120000; // 2 minutes max
unsigned long lastMQTTAttempt = 0;
const unsigned long mqttRetryInterval = 10000; // 10s entre chaque tentative MQTT
bool wifiConfigLoaded = false;
bool apModeStarted = false;

void setupPin()
{
  pinMode(outputPin1, OUTPUT);
   pinMode(outputPin2, OUTPUT);
    pinMode(outputPin3, OUTPUT);
  pinMode(inputPin1, INPUT_PULLUP);
pinMode(inputPin0, INPUT_PULLUP);
  pinMode(inputPin3, INPUT_PULLUP);
}

void reconnecter()
{
  if (WiFi.status() == WL_CONNECTED)
  { // Vérifie si le Wi-Fi est bien rétabli
    if (!client.connected())
    {
      // reconnect();
      handleWiFiAndMQTT();
    }
    client.loop();
  }
}
void startAPMode()
{
  scanningWiFi = false; // Arrêt du scan
  Serial.println("Passage en mode AP...");
  digitalWrite(ledSTA, LOW);
  // ⚡ Arrêter toute connexion STA / scan en cours
  WiFi.disconnect(true); // true = purge des réseaux et stop scan
  delay(100);            // petit délai pour que le hardware libère le STA

  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
  String ssidAp = "SmartSwitchC1-" + macAddress.substring(0, 4);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAp.c_str(), "Fortico1234", 1);
  digitalWrite(ledSTA, LOW);
  digitalWrite(ledAp, HIGH);

  initWebSocket();
  server.begin();
}

void handleWiFiAndMQTT()
{
  // Charger la configuration Wi-Fi si pas déjà fait
  if (!wifiConfigLoaded)
  {
    wifiConfigLoaded = loadWiFiConfig();
    if (wifiConfigLoaded)
    {
      digitalWrite(ledAp, LOW);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), password.c_str());
      wifiDisconnectTime = millis();
      tryingWiFi = true;
      scanningWiFi = true; // Commence le scan
      Serial.println("Tentative de connexion au Wi-Fi...");
    }
  }

  // Vérifier l'état du Wi-Fi
  if (WiFi.status() != WL_CONNECTED)
  {
    if (!tryingWiFi && wifiConfigLoaded)
    {
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str());
      wifiDisconnectTime = millis();
      tryingWiFi = true;
      scanningWiFi = true; // Commence le scan
      Serial.println("⚠️ Wi-Fi déconnecté, tentative...");
    }

    else if (tryingWiFi && millis() - wifiDisconnectTime > wifiReconnectTimeout)
    {
      Serial.println("❌ Wi-Fi toujours pas dispo → suppression config WiFi + bascule en AP");

      // 🔥 Supprimer uniquement le fichier Wi-Fi
      if (LittleFS.begin())
      {
        if (LittleFS.exists("/wifi.txt"))
        {
          if (LittleFS.remove("/wifi.txt"))
          {
            Serial.println("✅ Fichier /wifi.txt supprimé !");
          }
          else
          {
            Serial.println("❌ Échec suppression /wifi.txt !");
          }
        }
        else
        {
          Serial.println("ℹ️ Aucun fichier Wi-Fi à supprimer.");
        }
        LittleFS.end();
      }

      tryingWiFi = false;
      scanningWiFi = false;
      apModeStarted = true;

      digitalWrite(ledSTA, LOW);

      // ✅ Vérifier si config AP spéciale existe
      if (loadWiFiConfigAp())
      {
        Serial.println("⚡ Démarrage en mode AP via config Ap !");
        WiFi.softAP(ssidAp.c_str(), passwordAp.c_str(), 1);
        initWebSocket();
        server.begin();
        digitalWrite(ledAp, HIGH);
      }
      else
      {
        startAPMode(); // Sinon AP par défaut
        digitalWrite(ledAp, HIGH);
      }

      return;
    }

    // Clignotement LED uniquement si le scan est actif
    if (scanningWiFi)
    {
      updateBlinkLED();
    }
  }
  else
  {
    // Wi-Fi OK
    if (tryingWiFi)
    {
      Serial.println("✅ Connecté à " + ssid);
      digitalWrite(ledSTA, HIGH);
      digitalWrite(ledAp, LOW);
      scanningWiFi = false; // Arrêt du scan

      // Supprimer device_role.txt si présent
      if (LittleFS.begin())
      {
        if (LittleFS.exists("/device_role.txt"))
        {
          if (LittleFS.remove("/device_role.txt"))
          {
            Serial.println("Fichier device_role.txt supprimé !");
          }
          else
          {
            Serial.println("Échec de suppression du fichier !");
          }
        }
        LittleFS.end();
      }
      else
      {
        Serial.println("❌ LittleFS non monté !");
      }
    }

    tryingWiFi = false;
    apModeStarted = false;
  }

  // Gestion AP si pas de config Wi-Fi du tout
  if (!wifiConfigLoaded && !apModeStarted)
  {
    if (deviceRole == "Slave")
    {
      WiFi.mode(WIFI_STA);
    }
    else if (loadWiFiConfigAp()) // ✅ encore ici
    {
      Serial.println("⚡ Démarrage en mode AP via config Ap (aucune config STA)");
      WiFi.softAP(ssidAp.c_str(), passwordAp.c_str(), 1);
      initWebSocket();
      server.begin();
      digitalWrite(ledAp, HIGH);
      apModeStarted = true;
    }
    else
    {
      digitalWrite(ledAp, HIGH);
      Serial.println("Aucune configuration Wi-Fi trouvée. Mode AP activé.");
      String macAddress = WiFi.macAddress();
      macAddress.replace(":", "");
      String ssidAp = "SmartSwitchC1-" + macAddress.substring(0, 4);
      WiFi.softAP(ssidAp.c_str(), "Fortico1234", 1);
      initWebSocket();
      server.begin();
      apModeStarted = true;
    }
  }

  // Vérifier MQTT seulement si Wi-Fi OK
  if (WiFi.status() == WL_CONNECTED && !apModeStarted && !client.connected())
  {
    if (millis() - lastMQTTAttempt > mqttRetryInterval)
    {
      Serial.println("Tentative de connexion MQTT...");
      if (client.connect(mqttClientId.c_str(), mqtt_user, mqtt_password))
      {
        Serial.println("✅ MQTT connecté !");
        Serial.print("Abonné au topic ");
        Serial.println(mqtt_topic);
        client.subscribe(mqtt_topic.c_str());
        client.loop();
        // Publier l'état MQTT
        String statusTopic = "smartSwitch C1/" + macAddress + "/status";
        String stateMessage = state1 ? "ON" : "OFF";
        client.publish(statusTopic.c_str(), stateMessage.c_str(), true);
      }
      else
      {
        Serial.print("❌ MQTT échec, code: ");
        Serial.println(client.state());
      }
      lastMQTTAttempt = millis();
    }
  }
}

void setupServer()
{
  client.setServer(mqtt_server, mqtt_port);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // Buffer sécurisé pour le payload
  char tempPayload[length + 1];
  memcpy(tempPayload, payload, length);
  tempPayload[length] = '\0';

  Serial.print("Message reçu sur ");
  Serial.print(topic);
  Serial.print(" : ");
  Serial.println(tempPayload);

  String messageStr = String(tempPayload);

  if (strcmp(topic, mqtt_topic.c_str()) == 0)
  {
    // a) Commande manuelle "state"
    if (messageStr == "state")
    {
      if (!horloge.isEnabled())
      {
        state1 = !state1;
        digitalWrite(D8, state1);
        Serial.print("État de D8 changé manuellement : ");
        Serial.println(state1 ? "ON" : "OFF");

        // Publier l'état MQTT
        String statusTopic = "smartSwitch C1/" + macAddress + "/status";
        String stateMessage = state1 ? "ON" : "OFF";
        client.publish(statusTopic.c_str(), stateMessage.c_str(), true);
        Serial.print("État publié sur MQTT : ");
        Serial.println(stateMessage);
      }
      else
      {
        Serial.println("⚠️ Mode horloge actif : commande 'state' ignorée pour éviter conflit.");
      }
    }
    // CODE POUR SWITCH ALL
    else if (messageStr == "ON1")
    {
      if (!horloge.isEnabled())
      {
        state1 = HIGH;
        digitalWrite(D8, state1);
        Serial.print("État de D8 changé manuellement : ");
        Serial.println(state1 ? "ON" : "OFF");

        // Publier l'état MQTT
        String statusTopic = "smartSwitch C1/" + macAddress + "/status";
        String stateMessage = state1 ? "ON" : "OFF";
        client.publish(statusTopic.c_str(), stateMessage.c_str(), true);
        Serial.print("État publié sur MQTT : ");
        Serial.println(stateMessage);
      }
      else
      {
        Serial.println("⚠️ Mode horloge actif : commande 'state' ignorée pour éviter conflit.");
      }
    }
    else if (messageStr == "OFF1")
    {
      if (!horloge.isEnabled())
      {
        state1 = LOW;
        digitalWrite(D8, state1);
        Serial.print("État de D8 changé manuellement : ");
        Serial.println(state1 ? "ON" : "OFF");

        // Publier l'état MQTT
        String statusTopic = "smartSwitch C1/" + macAddress + "/status";
        String stateMessage = state1 ? "ON" : "OFF";
        client.publish(statusTopic.c_str(), stateMessage.c_str(), true);
        Serial.print("État publié sur MQTT : ");
        Serial.println(stateMessage);
      }
      else
      {
        Serial.println("⚠️ Mode horloge actif : commande 'state' ignorée pour éviter conflit.");
      }
    }

    // b) Message JSON = config horloge ou config WiFi
    else if (messageStr.startsWith("{"))
    {
      StaticJsonDocument<256> doc;
      DeserializationError error = deserializeJson(doc, messageStr);

      if (error)
      {
        Serial.println("❌ Erreur parsing JSON !");
        return;
      }

      const char *type = doc["type"];

      // b1) Configuration horloge
      if (type == nullptr || String(type) != "setWifi")
      {
        String mode = getMode();
        if (mode == "offline")
        {
          Serial.println("⛔ Mode Offline actif : configuration horloge ignorée.");
          return;
        }

        Serial.println("✅ Mode Online : configuration horloge acceptée.");
        horloge.onMessage(tempPayload, length);
        if (horloge.isEnabled())
        {
          time_t now = time(nullptr);
          struct tm localTime;
          localtime_r(&now, &localTime);

          // Convertir heures début/fin en minutes
          int currentMinutes = localTime.tm_hour * 60 + localTime.tm_min;
          int debutMinutes = horloge.heureToMinutes(horloge.getHeureDebut());
          int finMinutes = horloge.heureToMinutes(horloge.getHeureFin());

          if (currentMinutes == debutMinutes && state1 == LOW)
          {
            state1 = HIGH;
            digitalWrite(outputPin1, HIGH);
            Serial.println("⏰ Heure de début : sortie HIGH");
            nouvelEtat();
          }
          else if (currentMinutes == finMinutes && state1 == HIGH)
          {
            state1 = LOW;
            digitalWrite(outputPin1, LOW);
            Serial.println("⏰ Heure de fin : sortie LOW");
            nouvelEtat();
          }
        }

        else
        {
          Serial.println("⚠️ Mode horloge désactivé, état automatique non appliqué.");
        }
      }
      // b2) Configuration WiFi
      else if (String(type) == "setWifi")
      {
        const char *newSSID = doc["ssid"];
        const char *newPassword = doc["password"];

        if (newSSID && newPassword)
        {
          Serial.println("📡 Nouvelle configuration WiFi reçue via MQTT :");
          Serial.printf("SSID: %s\n", newSSID);
          Serial.printf("Password: %s\n", newPassword);

          saveWiFiConfig(newSSID, newPassword);
          delay(200); // laisser le temps d'écrire en flash

          WiFi.disconnect(true);
          delay(200);

          // ✅ Lancement de la nouvelle connexion
          WiFi.mode(WIFI_STA);
          WiFi.begin(newSSID, newPassword);

          wifiDisconnectTime = millis(); // moment où on a lancé la tentative
          tryingWiFi = true;             // on active le flag de tentative
          scanningWiFi = true;           // active le clignotement

          Serial.println("🔄 Nouvelle tentative de connexion Wi-Fi en arrière-plan...");
        }

        else
        {
          Serial.println("❌ Champs SSID ou Password manquants !");
        }
      }
    }

    // c) URL OTA
    else if (messageStr.startsWith("http"))
    {
      messageStr.trim();            // supprime espaces, \r, \n au début/fin
      messageStr.replace("\r", ""); // supprime tous les \r internes
      messageStr.replace("\n", ""); // supprime tous les \n internes

      static char otaUrl[256];
      messageStr.toCharArray(otaUrl, sizeof(otaUrl));
      otaUrl[sizeof(otaUrl) - 1] = '\0'; // sécurité : terminaison manuelle

      Serial.print("📩 URL reçue pour OTA : ");
      Serial.println(otaUrl);

      checkForUpdates(otaUrl);
      Serial.print("DEBUG ASCII URL: ");
      for (int i = 0; otaUrl[i] != '\0'; i++)
      {
        Serial.print("[");
        Serial.print((int)otaUrl[i]);
        Serial.print("]");
      }
      Serial.println();
    }

    // d) Formatage SPIFFS
    else if (messageStr == "format")
    {
      Serial.println("🧨 Commande de formatage reçue via MQTT !");
      delay(500);

      if (LittleFS.begin())
      {
        bool success = LittleFS.format();
        LittleFS.end();

        if (success)
        {
          Serial.println("✅ Formatage SPIFFS réussi. Redémarrage...");
          client.publish(mqtt_topic.c_str(), "FORMAT_OK", true);
          delay(1000);
          ESP.restart();
        }
        else
        {
          Serial.println("❌ Formatage SPIFFS échoué !");
          client.publish(mqtt_topic.c_str(), "FORMAT_FAIL", true);
        }
      }
      else
      {
        Serial.println("❌ Échec d'initialisation SPIFFS !");
        client.publish(mqtt_topic.c_str(), "FORMAT_INIT_FAIL", true);
      }
    }
    // e) restarte module
    else if (messageStr == "reset")
    {
      Serial.println("Commande reset reçue, redémarrage...");
      delay(200);    // petite pause pour terminer les logs
      ESP.restart(); // redémarre l'ESP
    }
    // f) Demande de version firmware
    else if (messageStr == "version")
    {
      String statusTopic = "smartSwitch C1/" + macAddress + "/status";
      client.publish(statusTopic.c_str(), FIRMWARE_VERSION, true);
      Serial.println("📤 Version firmware envoyée via MQTT : " + String(FIRMWARE_VERSION));
    }
  }
}

// 🔹 Fonction de mise à jour OTA

void checkForUpdates(const char *firmware_url)
{
    espClient.setInsecure(); // Ignore SSL

    Serial.print("🚀 Vérification de mise à jour avec l'URL : ");
    Serial.println(firmware_url);

    HTTPClient http;
    http.begin(espClient, firmware_url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Suivre les redirections

    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK)
    {
        Serial.println("✅ URL finale OK, lancement OTA...");
        t_httpUpdate_return ret = ESPhttpUpdate.update(espClient, firmware_url);
        switch (ret)
        {
        case HTTP_UPDATE_FAILED:
            Serial.printf("❌ Échec mise à jour: %s\n", ESPhttpUpdate.getLastErrorString().c_str());
            break;
        case HTTP_UPDATE_NO_UPDATES:
            Serial.println("✅ Pas de mise à jour disponible.");
            break;
        case HTTP_UPDATE_OK:
            Serial.println("✅ Mise à jour réussie ! Redémarrage...");
            ESP.restart();
            break;
        }
    }
    else
    {
        Serial.printf("❌ HTTP GET échoué, code = %d\n", httpCode);
    }

    http.end();
}

void setupCallBack()
{
  client.setCallback(callback);
  espClient.setInsecure(); // Désactive la vérification des certificats
                           // 🔹 Attendre la connexion MQTT avant de publier
  if (WiFi.status() == WL_CONNECTED)
  { // Vérifie si le Wi-Fi est bien rétabli
    if (!client.connected())
    {
      handleWiFiAndMQTT();
    }
    client.loop();
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    // Récupérer l'adresse MAC
    String macAddress = WiFi.macAddress();

    // 🔹 Publier l'adresse MAC sur un topic dédié (avec message retained)
    String macTopic = "smartSwitch C1/macAddress";
    client.publish(macTopic.c_str(), macAddress.c_str(), true);

    // 🔹 Publier l'état au démarrage
    String statusTopic = "smartSwitch C1/" + macAddress + "/status";
    String initialStateMessage = state1 ? "ON" : "OFF";
    client.publish(statusTopic.c_str(), initialStateMessage.c_str(), true); // Message retain

    Serial.print("Adresse MAC publiée sur MQTT : ");
    Serial.println(macAddress);
    Serial.print("État initial publié sur MQTT : ");
    Serial.println(initialStateMessage);
  }
}
void nouvelEtat()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    String statusTopic = "smartSwitch C1/" + macAddress + "/status";
    String stateMessage;
    if (state1 == HIGH)
    {
      stateMessage = "ON";
    }
    else
      stateMessage = "OFF";
    client.publish(statusTopic.c_str(), stateMessage.c_str(), false); // true = message retained
    Serial.print("État publié sur MQTT nouvelle etat : ");
    Serial.println(stateMessage);
  }
}

void outputPin()
{
  digitalWrite(outputPin1, state1 ? HIGH : LOW);
  if (state1 == HIGH)
    value = '1';
  else
    value = '0';

      digitalWrite(outputPin2, state2 ? HIGH : LOW);
  if (state2 == HIGH)
    value = "11";
  else
    value = "01";

      digitalWrite(outputPin3, state3 ? HIGH : LOW);
  if (state3 == HIGH)
    value = "12";
  else
    value = "02";
}
void inputCheck()
{

  lws_status1 = digitalRead(inputPin1);
  lws_status2 = digitalRead(inputPin3);
 lws_status3 = digitalRead(inputPin0);
  if (!horloge.isEnabled())
  {
    if (lws_status1 != old_lws_status1)
    {
      state1 = !state1;
      old_lws_status1 = lws_status1;
      nouvelEtat();
      value = (state1 == HIGH) ? '0' : '1';
      notifyClients();
    }
    else if (lws_status2 != old_lws_status2)
    {
      state2 = !state2;
      old_lws_status2 = lws_status2;
      nouvelEtat();
      value = (state2 == HIGH) ? "01" : "11";
      notifyClients();
    }
        else if (lws_status3 != old_lws_status3)
    {
      state3 = !state3;
      old_lws_status3 = lws_status3;
      nouvelEtat();
      value = (state3 == HIGH) ? "02" : "12";
      notifyClients();
    }
  }
}