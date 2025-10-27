// MAIN
#define MQTT_MAX_PACKET_SIZE 1024 // Ou 1024 pour être vraiment large. 768 devrait suffire.
#include <PubSubClient.h>         // Cette inclusion doit venir après le define
#include <ESP8266HTTPClient.h>
#include <ESPAsyncTCP.h>
#include <espnow.h>
#include <LittleFS.h> // Ajouté pour LittleFS
#include "led_sta.h"
#include "led_ap.h"
#include "mqtt_conf.h"
#include "websocketConf.h"
#include "slaveConf.h"
#include "otaOffline.h"
#include "HorlogeModule.h"
#include "mode_conf.h"
#include "config.h"
const char *FIRMWARE_VERSION = "1.0.2"; // Dernier mise à jour 27 Octobre 2025 By Jean Fortuno

#define ledSTA D3 // Définir la broche de la LED
#define ledAp D4

unsigned long previousMillisFirebase = 0;
unsigned long firebaseInterval = 0;
unsigned long lastInputTime = 0;
const long interval = 500;
unsigned long previousMillis = 0;
unsigned long ledBlinkInterval = 500;
bool led1State = LOW;
bool led2State = HIGH;
HorlogeModule horloge; // Déclaration globale connue ici
bool previousState = false;
bool ntpSynced = false;
unsigned long lastCheckTime = 0; // à déclarer en global
// --- Variables globales pour NTP ---
unsigned long ntpStartMillis = 0; // moment où la tentative NTP a commencé
unsigned long ntpStartTime = 0;
bool ntpInProgress = false;
unsigned long previousMillisTime = 0;
const long intervalTime = 1000; // 1 seconde

// Fix for the invisible character here! Ensure it's a regular space.
const unsigned long CHECK_INTERVAL = 500; // 5 secondes

// --- Start of Fix for setupPins() ---
// Move the definition of setupPins() BEFORE setup()
void setupPins()
{
  setupBlinkLED();
  setupFadeLED();
  setupPin();
  setupInput();
}
// --- End of Fix for setupPins() ---
void afficherHeureLocale()
{
  time_t now = time(nullptr);
  struct tm localTime;
  if (!localtime_r(&now, &localTime))
    return;

  char buf[30];
  strftime(buf, sizeof(buf), "%H:%M:%S %d/%m/%Y", &localTime);
  Serial.printf("🕒 Heure locale : %s\n", buf);
}

void startNTP()
{
  Serial.println("\n--- Démarrage synchronisation NTP ---");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
  ntpStartTime = millis();
  ntpInProgress = true;
}

void handleNTP()
{
  if (!ntpInProgress)
    return;

  time_t now = time(nullptr);
  if (now >= 10000)
  {
    Serial.println("✅ Heure NTP synchronisée !");
    horloge.appliquerTimezone();
    afficherHeureLocale();
    ntpSynced = true;
    ntpInProgress = false;
  }
  else if (millis() - ntpStartTime > 10000) // Timeout 10s
  {
    Serial.println("❌ Erreur : Impossible de synchroniser l'heure NTP.");
    ntpInProgress = false;
  }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- Démarrage du système ---");

    // --- Initialisation LittleFS ---
    if (!LittleFS.begin())
    {
        Serial.println("❌ LittleFS non monté ! Formatage en cours...");
        LittleFS.format();
        if (!LittleFS.begin())
        {
            Serial.println("❌ Échec du montage après formatage !");
            while (true)
                delay(100);
        }
    }
    Serial.println("✅ LittleFS monté avec succès");

    // --- Charger configuration horloge ---
    if (horloge.chargerDepuisLittleFS("/configHorloge.json"))
    {
        Serial.println("✅ Configuration horloge chargée depuis LittleFS");
        horloge.appliquerTimezone();
    }
    else
    {
        Serial.println("⚠️ Aucune configuration horloge trouvée, valeurs par défaut utilisées");
    }

    // --- Affichage infos LittleFS ---
    FSInfo fs_info;
    LittleFS.info(fs_info);
    Serial.printf("LittleFS Total: %u bytes, Used: %u bytes, Free: %u bytes\n",
                  fs_info.totalBytes, fs_info.usedBytes, fs_info.totalBytes - fs_info.usedBytes);

    // --- Test écriture ---
    File testFile = LittleFS.open("/test.txt", "w");
    if (testFile)
    {
        testFile.println("Test write");
        testFile.close();
        Serial.println("✅ Écriture test réussie dans /test.txt");
    }
    else
    {
        Serial.println("❌ Impossible d'écrire dans /test.txt");
        while (true)
            delay(100);
    }

    // --- Démarrage NTP ---
    startNTP();

    // --- Initialisation services ---
    initWebSocket();
    setupPins();
    setupServer();
    deficeRole();
    setupCallBack();
    initOTA();
    Server();
    server.begin();
    Serial.println("Serveur démarré !");

    Serial.println("--- Fin du setup ---");
}


void horlogeUpdate()
{
  if (!horloge.isEnabled())
    return;

  time_t now = time(nullptr);
  struct tm localTime;
  if (!localtime_r(&now, &localTime))
    return;

  bool shouldBeOn = horloge.isActive(localTime);
  if (shouldBeOn != state1)
  {
    state1 = shouldBeOn;
    digitalWrite(outputPin1, state1 ? HIGH : LOW);
    nouvelEtat();
    Serial.printf("⏰ Mise à jour périodique état : %s\n", state1 ? "ON" : "OFF");
  }
}

void loop()
{
  handleWiFiAndMQTT(); // gestion Wi-Fi et MQTT non bloquante
  handleNTP();         // NTP non bloquant
  client.loop();       // MQTT loop
  inputCheck();        // boutons
  formatage();         // formatage SPIFFS si déclenché
  outputPin();         // mise à jour des sorties
  ArduinoOTA.handle(); // OTA

  // Gestion horloge
  if (getMode() != "offline")
  {
    unsigned long nowMillis = millis();
    if (nowMillis - lastCheckTime > CHECK_INTERVAL)
    {
      lastCheckTime = nowMillis;
      horlogeUpdate();
    }
  }

  // Gestion du rôle du device si Wi-Fi déconnecté
  if (deviceRole == "Master" && WiFi.status() != WL_CONNECTED)
    modeMaster();
  else if (deviceRole == "Slave" && WiFi.status() != WL_CONNECTED)
    modeSlave();
}
