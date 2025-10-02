
// PARTY WEBSOCKET
#include "websocketConf.h"
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include "led_sta.h"
#include "led_ap.h"
#include <espnow.h>
#include <ESP8266WiFi.h>
#include "slaveConf.h"
#include "mqtt_conf.h"
#include "StateResponse.h"
#include "mode_conf.h"
bool buttonState;
bool lastButtonState = HIGH;
unsigned long buttonPressStart = 0;
bool formatageSpiffs = LOW;
#define inputPin2 D0
#define HOLD_TIME 3000
// #define ledAp D3
String value;
String valueS5, valueS6, valueS7, valueS8, valueS9;
#define MAX_SLAVES 9
uint8_t masterMacAddr[] = {0x52, 0x02, 0x91, 0x5A, 0x22, 0xEA};
uint8_t slaveMacAddrs[MAX_SLAVES][6];
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
int slaveCount = 0;
struct SlaveData
{
    uint8_t id;   // Index de l'esclave (1, 2, 3, ...)
    uint8_t type; // Type de message (ex: 0x01 = toggle)
    bool state;   // État de la sortie (true = ON, false = OFF)
};

SlaveData slaveData[MAX_SLAVES]; // Tableau des états esclaves
int numRegisteredSlaves = 0;     // Compteur des esclaves connus

String ssid = "";
String password = "";

String ssidAp = "";
String passwordAp = "";

String deviceRole = "";

bool initLittleFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("Erreur lors de l'initialisation de LittleFS");
        return false;
    }
    return true;
}

void setupInput()
{
    pinMode(inputPin2, INPUT_PULLUP);
}

void formatage()
{
    buttonState = digitalRead(inputPin2);

    // Détection du début de l'appui
    if (buttonState == LOW && lastButtonState == HIGH)
    {
        buttonPressStart = millis(); // Enregistre le temps de début de l'appui
    }

    // Détection de la fin de l'appui
    if (buttonState == HIGH && lastButtonState == LOW)
    {
        unsigned long pressDuration = millis() - buttonPressStart; // Durée de l'appui

        if (pressDuration >= HOLD_TIME)
        {
            Serial.println("Formatage LittleFS...");
            LittleFS.end(); // Arrêter LittleFS avant de formater
            //                             // CLIGNOTEMENT DE LED
            // for (int i = 0; i < 6; i++) // 3 clignotements (6 états ON/OFF)
            // {
            //     formatBlinkLED();
            // }
            if (LittleFS.format())
            {
                Serial.println("LittleFS formaté avec succès !");
            }
            else
            {
                Serial.println("Erreur de formatage LittleFS !");
            }

            Serial.println("Redémarrage...");
            delay(100); // Attente avant redémarrage
            ESP.restart();
        }
        else
        {
            Serial.println("Redémarrage simple...");
            delay(100); // Attente avant redémarrage
            ESP.restart();
        }
    }

    lastButtonState = buttonState; // Sauvegarde l'état précédent du bouton
}
void formatageForce()
{
    Serial.println("Formatage distant déclenché...");
    LittleFS.end();
    if (LittleFS.format())
    {
        Serial.println("LittleFS formaté avec succès !");
    }
    else
    {
        Serial.println("Erreur de formatage LittleFS !");
    }
    delay(100);
    ESP.restart();
}

void saveWiFiConfig(const char *ssid, const char *password)
{
    if (!LittleFS.begin())
    {
        Serial.println("❌ LittleFS non monté !");
        return;
    }

    File configFile = LittleFS.open("/wifi.txt", "w");
    if (configFile)
    {
        configFile.println(ssid);
        configFile.println(password);
        configFile.close();
        Serial.println("✅ Configuration WiFi sauvegardée !");
    }
    else
    {
        Serial.println("❌ Erreur d'ouverture /wifi.txt pour écriture.");
    }

    LittleFS.end(); // Important : démonter après
}

bool loadWiFiConfig()
{
    if (LittleFS.exists("/wifi.txt"))
    {
        File configFile = LittleFS.open("/wifi.txt", "r");
        if (configFile)
        {
            ssid = configFile.readStringUntil('\n');
            password = configFile.readStringUntil('\n');
            ssid.trim();
            password.trim();
            configFile.close();
            return true;
        }
    }
    return false;
}

void saveWiFiConfigAp(const char *ssidAp, const char *passwordAp)
{
    File configFile = LittleFS.open("/wifiAp.txt", "w");
    if (configFile)
    {
        configFile.println(ssidAp);
        configFile.println(passwordAp);
        configFile.close();
    }
}

bool loadWiFiConfigAp()
{
    if (LittleFS.exists("/wifiAp.txt"))
    {
        File configFile = LittleFS.open("/wifiAp.txt", "r");
        if (configFile)
        {
            ssidAp = configFile.readStringUntil('\n');
            passwordAp = configFile.readStringUntil('\n');
            ssidAp.trim();
            passwordAp.trim();
            configFile.close();
            return true;
        }
    }
    return false;
}

void saveToSPIFFS(const String &filename, const String &data)
{
    File file = LittleFS.open(filename, "w");
    if (file)
    {
        file.print(data);
        file.close();
        Serial.println("Donnée sauvegardée : " + data);
    }
    else
    {
        Serial.println("Erreur lors de la sauvegarde.");
    }
}

String loadFromSPIFFS(const String &filename)
{
    File file = LittleFS.open(filename, "r");
    if (!file)
    {
        Serial.println("Fichier non trouvé : " + filename);
        return "";
    }
    String data = file.readString();
    file.close();
    Serial.println("Donnée chargée : " + data);
    return data;
}

bool loadMacFromSPIFFS(uint8_t *mac)
{
    String macSlaveStr = loadFromSPIFFS("/slave_mac.txt");

    if (macSlaveStr.length() != 17)
    {
        return false;
    }
    for (int i = 0; i < 6; i++)
    {
        String byteStr = macSlaveStr.substring(i * 3, i * 3 + 2);
        mac[i] = strtol(byteStr.c_str(), NULL, 16);
    }

    return true;
}

void loadAllSlavesFromSPIFFS()
{
    String allSlaves = loadFromSPIFFS("/slave_mac.txt"); // Charger le contenu du fichier
    if (allSlaves.length() > 0)
    {
        int index = 0;
        int start = 0;
        int end;

        while (index < MAX_SLAVES) // Limiter le nombre d'esclaves à MAX_SLAVES
        {
            end = allSlaves.indexOf('\n', start); // Chercher la fin de la ligne
            if (end == -1)
                break; // Fin de fichier, arrêter la boucle

            String macString = allSlaves.substring(start, end); // Extraire l'adresse MAC
            start = end + 1;                                    // Passer à la prochaine ligne

            // Vérifier que l'adresse MAC est de la bonne longueur (XX:XX:XX:XX:XX:XX)
            if (macString.length() == 17)
            {
                for (int i = 0; i < 6; i++)
                {
                    String byteStr = macString.substring(i * 3, i * 3 + 2);      // Extraire chaque octet
                    slaveMacAddrs[index][i] = strtol(byteStr.c_str(), NULL, 16); // Convertir en entier
                }
                Serial.println("Esclave chargé: " + macString); // Afficher l'adresse MAC
                index++;
            }
            else
            {
                Serial.println("Adresse MAC invalide: " + macString); // Si l'adresse MAC est incorrecte
            }
        }

        slaveCount = index; // Mettre à jour le compteur d'esclaves
        Serial.print("Nombre d'esclaves chargés: ");
        Serial.println(slaveCount); // Afficher le nombre d'esclaves chargés
    }
    else
    {
        Serial.println("Aucun esclave trouvé dans le fichier."); // Si le fichier est vide ou non trouvé
    }
}

void saveAllSlavesToSPIFFS()
{
    String allSlaves = "";
    for (int i = 0; i < slaveCount; i++)
    {
        String macString = "";
        for (int j = 0; j < 6; j++)
        {
            // Formater chaque octet avec un zéro devant si nécessaire
            macString += (slaveMacAddrs[i][j] < 16 ? "0" : "") + String(slaveMacAddrs[i][j], HEX);
            if (j < 5)
            {
                macString += ":"; // Ajouter ":" entre les octets
            }
        }
        allSlaves += macString + "\n"; // Ajouter l'adresse MAC au string
    }
    saveToSPIFFS("/slave_mac.txt", allSlaves); // Sauvegarder le tout dans un fichier
}

void notifyClients()
{
    ws.textAll(String(value));
}
bool isMacInList(uint8_t *mac)
{
    for (int i = 0; i < slaveCount; i++)
    {
        if (memcmp(slaveMacAddrs[i], mac, 6) == 0)
        {
            return true; // L'adresse MAC est déjà dans la liste
        }
    }
    return false; // L'adresse MAC n'est pas dans la liste
}
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->opcode == WS_TEXT)
    {
        data[len] = '\0';
        String message = String((char *)data);

        if (message.startsWith("setWifi"))
        {
            Serial.println("setWifi bien reçu");

            int firstComma = message.indexOf(",");
            if (firstComma > 7)
            {
                String ssid = message.substring(7, firstComma);
                String password = message.substring(firstComma + 1);

                // Sauvegarder les nouveaux identifiants
                saveWiFiConfig(ssid.c_str(), password.c_str());

                Serial.println("Nouveau SSID: " + ssid);
                Serial.println("Nouveau mot de passe: " + password);

                ws.textAll("Connexion au nouveau WiFi...");

                // Déconnexion et reconnexion
                WiFi.disconnect(true); // true = effacer les anciens paramètres
                delay(1000);
                WiFi.begin(ssid.c_str(), password.c_str());

                // Attente de la connexion (max 10 secondes)
                int attempts = 0;
                while (WiFi.status() != WL_CONNECTED && attempts < 20)
                {
                    delay(500);
                    Serial.print(".");
                    attempts++;
                }

                if (WiFi.status() == WL_CONNECTED)
                {
                    Serial.println("\nConnexion réussie !");
                    Serial.print("IP: ");
                    Serial.println(WiFi.localIP());
                    ws.textAll("Connexion réussie ! IP: " + WiFi.localIP().toString());
                }
                else
                {
                    Serial.println("\nÉchec de la connexion.");
                    ws.textAll("Erreur : Connexion échouée !");
                }
            }
            else
            {
                Serial.println("Format invalide pour le WiFi");
                ws.textAll("Erreur : format invalide");
            }
        }
        //
        if (message.startsWith("confAp"))
        {
            Serial.println("Point d'accès réçu bien reçu");

            int firstCommaAp = message.indexOf(",");
            if (firstCommaAp > 7)
            {
                String ssidAp = message.substring(6, firstCommaAp);
                String passwordAp = message.substring(firstCommaAp + 1);

                // Sauvegarder les nouveaux identifiants
                saveWiFiConfigAp(ssidAp.c_str(), passwordAp.c_str());

                Serial.println("Nouveau SSIDAp: " + ssidAp);
                Serial.println("Nouveau mot de passeAp: " + passwordAp);

                ws.textAll("Connexion au nouveau WiFi...");

                // Déconnexion et reconnexion
                WiFi.disconnect(true); // true = effacer les anciens paramètres
                delay(1000);

                WiFi.softAP(ssidAp.c_str(), passwordAp.c_str());
                // server.on("/", handleRoot);
                // server.on("/save", HTTP_POST, handleSave);
                initWebSocket();
                server.begin();
            }
        }

        if (message.startsWith("setMac"))
        {
            String macSlaveString = message.substring(7);

            // Supprimer les deux-points ou tout autre séparateur de la chaîne
            macSlaveString.replace(":", "");

            // Vérifier que la chaîne a la bonne longueur (12 caractères)
            if (macSlaveString.length() != 12)
            {
                Serial.println("Adresse MAC invalide.");
                return;
            }

            // Convertir la chaîne en adresse MAC
            uint8_t newSlaveMac[6];
            for (int i = 0; i < 6; i++)
            {
                newSlaveMac[i] = strtol(macSlaveString.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
            }

            // Vérifier si l'adresse MAC est déjà présente
            if (isMacInList(newSlaveMac))
            {
                Serial.println("L'adresse MAC est déjà ajoutée : " + macSlaveString);
                return; // Sortir si l'adresse MAC est un doublon
            }

            // Vérifier si on peut ajouter un nouvel esclave
            if (slaveCount < MAX_SLAVES)
            {
                // Sauvegarder l'adresse MAC sous forme de chaîne avec les zéros
                String formattedMac = "";
                for (int i = 0; i < 6; i++)
                {
                    String byteHex = String(newSlaveMac[i], HEX);
                    if (byteHex.length() == 1)
                    {
                        byteHex = "0" + byteHex; // Ajouter un zéro si nécessaire
                    }
                    formattedMac += byteHex;
                    if (i < 5)
                        formattedMac += ":"; // Ajouter les séparateurs
                }

                // Afficher l'adresse MAC formatée
                Serial.println("Nouvelle adresse MAC formatée : " + formattedMac);

                // Sauvegarder l'adresse MAC formatée dans la liste des esclaves
                memcpy(slaveMacAddrs[slaveCount], newSlaveMac, sizeof(newSlaveMac));
                slaveCount++;
                saveAllSlavesToSPIFFS(); // Sauvegarder toutes les adresses MAC
                Serial.println("Nouvel esclave ajouté : " + formattedMac);
            }
            else
            {
                Serial.println("Limite d'esclaves atteinte.");
            }
        }

        else if (message.startsWith("toggle_slave "))
        {
            // Extraire l'indice de l'esclave
            int slaveIndex = message.substring(13).toInt(); // Récupérer l'index après "toggle_slave"

            // Vérifier que l'indice est valide
            if (slaveIndex >= 1 && slaveIndex <= slaveCount)
            {
                uint8_t msg[] = {0xFF};

                // Afficher l'adresse MAC de l'esclave concerné
                String macAddress = "";
                for (int i = 0; i < 6; i++)
                {
                    // Convertir chaque octet en hexadécimal et ajouter un zéro si nécessaire
                    String byteHex = String(slaveMacAddrs[slaveIndex - 1][i], HEX);
                    if (byteHex.length() == 1)
                    {
                        byteHex = "0" + byteHex; // Ajouter un zéro devant si le byte est inférieur à 16
                    }
                    macAddress += byteHex;
                    if (i < 5)
                        macAddress += ":"; // Ajouter un séparateur entre les octets
                }

                // Afficher l'adresse MAC dans le moniteur série
                Serial.println("Message envoyé à l'esclave " + String(slaveIndex) + " avec l'adresse MAC : " + macAddress);

                // Envoi au bon esclave
                esp_now_send(slaveMacAddrs[slaveIndex - 1], msg, sizeof(msg));
                Serial.println("Message envoyé à l'esclave " + String(slaveIndex));
            }
            else
            {
                Serial.println("Index d'esclave invalide : " + String(slaveIndex) + ". Doit être entre 1 et " + String(slaveCount));
            }
        }
        //___________________________fonction de mise à jour
        else if (message == "firsttoggle")
        {
            // Envoi de l'état local
            value = (state1 == HIGH) ? '0' : '1';
            value = (state2 == HIGH) ? "01" : "11";
            value = (state3 == HIGH) ? "02" : "12";
            notifyClients();

            // Envoi de l'adresse MAC
            String mac = WiFi.macAddress();
            ws.textAll("macAddress:" + mac);

            // Envoi des états des esclaves
            for (int i = 0; i < numRegisteredSlaves; i++)
            {
                String slaveMsg = (slaveData[i].state ? "A" : "B") + String(slaveData[i].id);
                ws.textAll(slaveMsg); // Même format que notifyClients() utilise
            }
        }

        else if (message == "toggleAll")
        {
            state1 = !state1;
            notifyClients();
            uint8_t msg[] = {0xFF};
            for (int i = 0; i < slaveCount; i++)
            {
                esp_now_send(slaveMacAddrs[i], msg, sizeof(msg));
            }
        }
        else if (message == "ON")
        {
            state1 = HIGH;
            state2 = HIGH;
            state3 = HIGH;
            value = (state1 == HIGH) ? "0" : "1";
            value = (state2 == HIGH) ? "01" : "11";
            value = (state3 == HIGH) ? "02" : "12";
            notifyClients();
            uint8_t msg[] = {0xAA};
            for (int i = 0; i < slaveCount; i++)
            {
                esp_now_send(slaveMacAddrs[i], msg, sizeof(msg));
            }
        }
        else if (message == "OFF")
        {
             state1 = LOW;
            state2 = LOW;
            state3 = LOW;
            value = (state1 == LOW) ? "1" : "0";
            value = (state2 == LOW) ? "11" : "01";
            value = (state3 == LOW) ? "12" : "02";
            notifyClients();
            uint8_t msg[] = {0xBB};
            for (int i = 0; i < slaveCount; i++)
            {
                esp_now_send(slaveMacAddrs[i], msg, sizeof(msg));
            }
        }

        else if (message.startsWith("formatage_slave "))
        {
            int slaveIndex = message.substring(16).toInt(); // Récupère X après "formatage_slave "

            if (slaveIndex >= 1 && slaveIndex <= slaveCount)
            {
                uint8_t msg[] = {0xEF};                                        // Code de formatage
                esp_now_send(slaveMacAddrs[slaveIndex - 1], msg, sizeof(msg)); // slaveIndex - 1 car tableau commence à 0

                Serial.println("Commande de formatage envoyée à l'esclave " + String(slaveIndex));
            }
            else
            {
                Serial.println("Index d'esclave invalide : " + String(slaveIndex));
            }
        }

        else if (message == "toggle1")
        {
            state1 = !state1;
            notifyClients();
            Serial.println("toggle bien réçu");
        }

          else if (message == "toggle2")
        {
            state2 = !state2;
            notifyClients();
            Serial.println("toggle bien réçu");
        }

          else if (message == "toggle3")
        {
            state3 = !state3;
            notifyClients();
            Serial.println("toggle bien réçu");
        }
        else if (message == "setMaster")
        {
            deviceRole = "Master";
            saveToSPIFFS("/device_role.txt", deviceRole);
            configureEspNow();
            ESP.restart();
        }
        else if (message == "setSlave")
        {
            deviceRole = "Slave";
            saveToSPIFFS("/device_role.txt", deviceRole);
            configureEspNow();
            ESP.restart();
        }

        else if (message == "Restarte")
        {
            Serial.println("Restarte en cours...");
            ESP.restart();
        }

        else if (message.startsWith("RestarteSlave"))
        {
            int slaveIndex = message.substring(16).toInt(); // Récupère X après "formatage_slave "

            if (slaveIndex >= 1 && slaveIndex <= slaveCount)
            {
                uint8_t msg[] = {0xAF};                                        // Code de formatage
                esp_now_send(slaveMacAddrs[slaveIndex - 1], msg, sizeof(msg)); // slaveIndex - 1 car tableau commence à 0

                Serial.println("Commande de restarte envoyée à l'esclave " + String(slaveIndex));
            }
            else
            {
                Serial.println("Index d'esclave invalide : " + String(slaveIndex));
            }
        }

        else if (message == "formatage")
        {
            Serial.println("Formatage bien reçu ");
            uint8_t msg[] = {0xEF};

            Serial.println("Nombre de slaves à formater : " + String(slaveCount));

            // Envoi aux esclaves
            for (int i = 0; i < slaveCount; i++)
            {
                esp_now_send(slaveMacAddrs[i], msg, sizeof(msg));
                delay(300); // Pause pour laisser chaque slave traiter
            }

            Serial.println("Ordres envoyés. Formatage du module master maintenant...");

            // Attention : pas de delay ici
            LittleFS.end(); // Toujours faire end() AVANT de formater

            bool success = LittleFS.format();
            Serial.println(success ? "Master : ✅ Formatage réussi !" : "Master : ❌ Échec formatage");

            Serial.println("Redémarrage du master...");
            delay(200); // Facultatif pour laisser le message s'imprimer
            ESP.restart();
        }
    }
}

void OnDataRecvS(uint8_t *mac, uint8_t *data, uint8_t len)
{
    Serial.print("Message ESP-NOW reçu du Master MAC: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(macStr);

    Serial.print(", Longueur: ");
    Serial.print(len);
    Serial.print(", Données: ");
    for (int i = 0; i < len; i++)
    {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    // Met à jour masterMacAddr si différent
    if (memcmp(masterMacAddr, mac, 6) != 0)
    {
        memcpy(masterMacAddr, mac, 6);
        Serial.println("Adresse MAC du master mise à jour dynamiquement.");

        // Ajout dynamique du master comme peer (ESP8266)
        if (esp_now_is_peer_exist(mac) == false)
        {
            // role = ESP_NOW_ROLE_COMBO permet RX/TX, channel = 0 (auto), pas de clé
            if (esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, 0, NULL, 0) == 0)
            {
                Serial.println("Master ajouté comme peer.");
            }
            else
            {
                Serial.println("Échec de l'ajout du peer.");
            }
        }
    }

    // Traitement des commandes
    if (len == 1 && data[0] == 0xFF)
    {
        state1 = !state1;
        Serial.println("Commande de basculement reçue du Master. Nouvel état : " + String(state1));
        digitalWrite(outputPin1, state1);

        // Préparation du retour
        StateResponse response;
        response.type = 0x01; // Code pour retour d’état
        response.state = state1;
        esp_now_send(masterMacAddr, (uint8_t *)&response, sizeof(response));
    }
    // ON ALL
    if (len == 1 && data[0] == 0xAA)
    {
        state1 = HIGH;
        Serial.println("Commande de basculement reçue du Master. Nouvel état : " + String(state1));
        digitalWrite(outputPin1, state1);

        // Préparation du retour
        StateResponse response;
        response.type = 0x01; // Code pour retour d’état
        response.state = state1;
        esp_now_send(masterMacAddr, (uint8_t *)&response, sizeof(response));
    }
    // OFF ALL
    if (len == 1 && data[0] == 0xBB)
    {
        state1 = LOW;
        Serial.println("Commande de basculement reçue du Master. Nouvel état : " + String(state1));
        digitalWrite(outputPin1, state1);

        // Préparation du retour
        StateResponse response;
        response.type = 0x01; // Code pour retour d’état
        response.state = state1;
        esp_now_send(masterMacAddr, (uint8_t *)&response, sizeof(response));
    }

    else if (len == 1 && data[0] == 0xEF)
    {
        bool formatSuccess = false;
        LittleFS.end();
        formatSuccess = LittleFS.format();
        esp_now_send(masterMacAddr, (uint8_t *)&formatSuccess, sizeof(formatSuccess));
        delay(100); // Donne le temps d'envoyer
        ESP.restart();
    }
}
// Traitemant des messages réçu via les ésclaves
void OnDataRecvM(uint8_t *mac, uint8_t *data, uint8_t len)
{
    Serial.print("Message ESP-NOW reçu de l'Esclave MAC: ");
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print(macStr);

    Serial.print(", Longueur: ");
    Serial.print(len);
    Serial.print(", Données: ");
    for (int i = 0; i < len; i++)
    {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();

    int slaveId = -1;
    for (int i = 0; i < slaveCount; i++)
    {
        if (memcmp(mac, slaveMacAddrs[i], 6) == 0)
        {
            slaveId = i + 1;
            break;
        }
    }

    if (slaveId == -1)
    {
        Serial.println("Esclave inconnu !");
        return;
    }

    // Traitement des données
    if (len == sizeof(StateResponse))
    {
        StateResponse *response = (StateResponse *)data;
        Serial.print("Réponse de l'esclave ");
        Serial.print(slaveId);
        Serial.print(": Type=");
        Serial.print(response->type, HEX);
        Serial.print(", État=");
        Serial.println(response->state ? "Allumé" : "Éteint");

        // 🔄 Mémorisation dans le tableau global
        bool updated = false;
        for (int i = 0; i < numRegisteredSlaves; i++)
        {
            if (slaveData[i].id == slaveId)
            {
                slaveData[i].type = response->type;
                slaveData[i].state = response->state;
                updated = true;
                break;
            }
        }
        if (!updated && numRegisteredSlaves < MAX_SLAVES)
        {
            slaveData[numRegisteredSlaves].id = slaveId;
            slaveData[numRegisteredSlaves].type = response->type;
            slaveData[numRegisteredSlaves].state = response->state;
            numRegisteredSlaves++;
        }

        // Notifier si toggle
        if (response->type == 0x01)
        {
            value = (response->state) ? "A" + String(slaveId) : "B" + String(slaveId);
            notifyClients();
        }
        else if (response->type == 0x02)
        {
            Serial.print("Réponse de formatage esclave. Résultat = ");
            Serial.println(response->state ? "✅" : "❌");

            if (response->state)
            {
                Serial.println("✅ Formatage réussi sur l'esclave. Suppression du module...");
                removeSlaveByMac(mac);
            }
            else
            {
                Serial.println("❌ Échec du formatage sur l'esclave.");
            }
        }
    }
    else if (len == sizeof(bool))
    {
        bool formatSuccess = *((bool *)data);
        Serial.print("Réponse de l'esclave ");
        Serial.print(slaveId);
        Serial.print(" : ");
        Serial.println(formatSuccess ? "✅ Formatage réussi." : "❌ Échec du formatage.");
    }

    else
    {
        Serial.println("Message reçu invalide.");
    }
}
void removeSlaveByMac(uint8_t *macToRemove)
{
    bool found = false;
    for (int i = 0; i < slaveCount; i++)
    {
        Serial.print("Comparaison avec esclave ");
        Serial.print(i);
        Serial.print(" : ");
        for (int j = 0; j < 6; j++)
        {
            Serial.printf("%02X ", slaveMacAddrs[i][j]);
        }
        Serial.println();

        if (memcmp(slaveMacAddrs[i], macToRemove, 6) == 0)
        {
            found = true;

            // Décaler les éléments suivants
            for (int j = i; j < slaveCount - 1; j++)
            {
                memcpy(slaveMacAddrs[j], slaveMacAddrs[j + 1], 6);
            }

            slaveCount--;

            Serial.println("✅ Esclave supprimé (MAC):");
            char macStr[18];
            snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                     macToRemove[0], macToRemove[1], macToRemove[2],
                     macToRemove[3], macToRemove[4], macToRemove[5]);
            Serial.println(macStr);

            saveAllSlavesToSPIFFS(); // Sauvegarder
            return;
        }
    }

    if (!found)
    {
        Serial.println("❌ Esclave à supprimer non trouvé !");
        Serial.print("MAC recherchée : ");
        for (int j = 0; j < 6; j++)
        {
            Serial.printf("%02X ", macToRemove[j]);
        }
        Serial.println();
    }
}

// Gestionnaire d’événements WebSocket (async)
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        // Envoie de l'adresse MAC dès que le client se connecte
        String mac = WiFi.macAddress();
        String message = "macAddress:" + mac;
        client->text(message);
        Serial.println("MAC envoyée au client : " + message);
    }
    else if (type == WS_EVT_DATA)
    {
        handleWebSocketMessage(arg, data, len);
    }
}

void initWebSocket()
{
    ws.onEvent(onEvent);
    server.addHandler(&ws);
}
void Server()
{
    // ... autres routes
    server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "application/json", "{\"status\":\"ok\"}"); });
}

// Fonction séparée pour le mode AP


void configureEspNow()
{

    if (deviceRole == "Master")
    {

        if (esp_now_init() != 0)
        {
            Serial.println("Erreur lors de l'initialisation d'ESP-NOW");
            return;
        }
        Serial.println("ESP-NOW initialisé pour master");

        esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
        esp_now_register_recv_cb(OnDataRecvM);

        // Charger les adresses MAC des esclaves depuis SPIFFS
        for (int i = 0; i < slaveCount; i++)
        {
            if (!esp_now_is_peer_exist(slaveMacAddrs[i]))
            {
                if (esp_now_add_peer(slaveMacAddrs[i], ESP_NOW_ROLE_SLAVE, 1, NULL, 0) == 0)
                {
                    Serial.println("Esclave " + String(i + 1) + " ajouté avec succès.");
                }
                else
                {
                    Serial.println("Erreur lors de l'ajout de l'esclave " + String(i + 1) + ".");
                }
            }
        }
    }
    else if (deviceRole == "Slave")
    {
        WiFi.mode(WIFI_AP_STA); // ← Important
        WiFi.softAP("SlaveESP", "12345678", 1);
        if (esp_now_init() != 0)
        {
            Serial.println("Erreur lors de l'initialisation d'ESP-NOW");
            return;
        }
        Serial.println("ESP-NOW initialisé ok pour slave");

        esp_now_set_self_role(ESP_NOW_ROLE_SLAVE); // uniquement ESP8266
        esp_now_register_recv_cb(OnDataRecvS);
    }
}

void deficeRole()
{
    // Charger le rôle et configurer ESP-NOW au démarrage
    deviceRole = loadFromSPIFFS("/device_role.txt");
    Serial.println("Rôle actuel : " + deviceRole);
    loadAllSlavesFromSPIFFS();
    configureEspNow(); // Configurer ESP-NOW après avoir chargé le rôle
    Serial.print("Rôle actuel du module : ");
    Serial.println(deviceRole);
    Serial.println("Esclaves enregistrés:");
    for (int i = 0; i < slaveCount; i++)
    {
        Serial.print("Esclave");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(slaveMacAddrs[i][0], HEX);
        Serial.print(":");
        Serial.print(slaveMacAddrs[i][1], HEX);
        Serial.print(":");
        Serial.print(slaveMacAddrs[i][2], HEX);
        Serial.print(":");
        Serial.print(slaveMacAddrs[i][3], HEX);
        Serial.print(":");
        Serial.print(slaveMacAddrs[i][4], HEX);
        Serial.print(":");
        Serial.println(slaveMacAddrs[i][5], HEX);
    }
}
