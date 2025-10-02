#include <ArduinoJson.h>
#include "HorlogeModule.h"
#include <LittleFS.h> // Fournit LittleFS et le type File
#include <FS.h>
#include "mqtt_conf.h"

bool HorlogeModule::isEnabled() const
{
    return enabled;
}
HorlogeModule::HorlogeModule()
    : enabled(false), heureDebut("--:--"), heureFin("--:--"), timezone("UTC") {}

void HorlogeModule::onMessage(const char *payload, size_t length)
{
    Serial.print(">>> onMessage reçu. Timezone AVANT parseJson: '");
    Serial.print(timezone);
    Serial.println("'");

    parseJson(payload, length);

    // Sauvegarde juste après le parsing JSON
    sauvegarderDansLittleFS("/configHorloge.json");

    Serial.print("Horloge config mise à jour. Timezone APRES parseJson: '");
    Serial.print(timezone);
    Serial.println("'");

    Serial.println(">>> onMessage appel de appliquerTimezone <<<");
    appliquerTimezone();
    Serial.println(">>> FIN onMessage <<<");
}

bool HorlogeModule::chargerDepuisLittleFS(const char *path)
{
    Serial.print("DEBUG LOAD: Tentative de chargement depuis : ");
    Serial.println(path);

    if (!LittleFS.exists(path))
    {
        Serial.println("DEBUG LOAD: ℹ️ Aucun fichier de configuration trouvé dans LittleFS. (Ce n'est pas une erreur en soi)");
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file)
    {
        Serial.println("DEBUG LOAD: ❌ Erreur ouverture fichier LittleFS pour lecture. (file == null)");
        return false;
    }

    // Affichez la taille du fichier pour vous assurer qu'il contient des données
    Serial.printf("DEBUG LOAD: Fichier ouvert. Taille du fichier: %u octets\n", file.size());

    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    file.close(); // Fermez le fichier après la tentative de désérialisation

    if (error)
    {
        Serial.print("DEBUG LOAD: ❌ Erreur JSON LittleFS lors du chargement : ");
        Serial.println(error.c_str());
        // Affichez le contenu brut du fichier pour inspection si le JSON est invalide
        Serial.println("DEBUG LOAD: Contenu brut du fichier (si la désérialisation échoue) :");
        File rawFile = LittleFS.open(path, "r");
        if (rawFile)
        {
            while (rawFile.available())
            {
                Serial.write(rawFile.read());
            }
            rawFile.close();
        }
        else
        {
            Serial.println("DEBUG LOAD: Impossible de rouvrir le fichier pour afficher le contenu brut.");
        }
        Serial.println("\n--- FIN Contenu brut ---");
        return false;
    }

    // Affichez le JSON parsé pour validation
    Serial.println("DEBUG LOAD: JSON parsé avec succès depuis LittleFS:");
    serializeJson(doc, Serial);
    Serial.println();

    enabled = doc["enabled"] | false;
    heureDebut = doc["heureDebut"] | "--:--";
    heureFin = doc["heureFin"] | "--:--";
    timezone = doc["timezone"] | "UTC";

    jours.clear();
    if (doc.containsKey("jours"))
    {
        for (JsonVariant v : doc["jours"].as<JsonArray>())
        {
            jours.push_back(String((const char *)v));
        }
    }

    Serial.println("DEBUG LOAD: ✅ Configuration horloge chargée depuis LittleFS et appliquée aux membres.");
    // Affichez les valeurs finales après chargement
    Serial.printf("DEBUG LOAD: enabled=%s, heureDebut=%s, heureFin=%s, timezone=%s\n",
                  enabled ? "true" : "false", heureDebut.c_str(), heureFin.c_str(), timezone.c_str());
    Serial.print("DEBUG LOAD: Jours chargés: ");
    for (const String &j : jours)
    {
        Serial.print(j + " ");
    }
    Serial.println();

    return true;
}

void HorlogeModule::parseJson(const char *payload, size_t length)
{
    Serial.println(">>> DEBUT parseJson <<<");
    Serial.print("Payload brut reçu : ");
    Serial.write(payload, length);
    Serial.println();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload, length);
    if (error)
    {
        Serial.print("Erreur JSON horloge: ");
        Serial.println(error.c_str());
        Serial.println(">>> FIN parseJson - Erreur <<<");
        return;
    }

    Serial.println("JSON parsé avec succès.");
    serializeJson(doc, Serial);
    Serial.println();

    enabled = doc["enabled"] | false;

    const char *hd = doc["heureDebut"].as<const char *>();
    const char *hf = doc["heureFin"].as<const char *>();

    if (hd && strlen(hd) == 5 && hd[2] == ':')
    {
        heureDebut = String(hd);
        heureDebut.trim();
    }
    else
    {
        Serial.println("⚠️ heureDebut invalide ou absente. Défaut '--:--'");
        heureDebut = "--:--";
    }

    if (hf && strlen(hf) == 5 && hf[2] == ':')
    {
        heureFin = String(hf);
        heureFin.trim();
    }
    else
    {
        Serial.println("⚠️ heureFin invalide ou absente. Défaut '--:--'");
        heureFin = "--:--";
    }

    jours.clear();
    if (doc.containsKey("jours"))
    {
        for (JsonVariant v : doc["jours"].as<JsonArray>())
        {
            jours.push_back(String((const char *)v));
        }
    }

    const char *tz = doc["timezone"].as<const char *>();
    if (tz)
    {
        timezone = String(tz);
    }
    else
    {
        timezone = "UTC";
    }

    Serial.print("DEBUG: timezone après l'affectation dans parseJson: '");
    Serial.print(timezone);
    Serial.println("'");
    Serial.println(">>> FIN parseJson - Succès <<<");
}

int HorlogeModule::jourToWDay(const String &jour) const
{
    if (jour.equalsIgnoreCase("dimanche"))
        return 0;
    if (jour.equalsIgnoreCase("lundi"))
        return 1;
    if (jour.equalsIgnoreCase("mardi"))
        return 2;
    if (jour.equalsIgnoreCase("mercredi"))
        return 3;
    if (jour.equalsIgnoreCase("jeudi"))
        return 4;
    if (jour.equalsIgnoreCase("vendredi"))
        return 5;
    if (jour.equalsIgnoreCase("samedi"))
        return 6;
    return -1;
}

int HorlogeModule::heureToMinutes(const String &h) const
{
    int sep = h.indexOf(':');
    if (sep < 0)
        return -1;
    int hh = h.substring(0, sep).toInt();
    int mm = h.substring(sep + 1).toInt();
    return hh * 60 + mm;
}

bool HorlogeModule::isActive(const struct tm &now) const
{
    if (!enabled)
    {
        Serial.println("DEBUG isActive: horloge désactivée");
        return false;
    }

    int wday = now.tm_wday;
    Serial.printf("DEBUG isActive: jour actuel (tm_wday) = %d\n", wday);

    bool jourOk = false;
    Serial.print("DEBUG isActive: jours configurés = ");
    for (const auto &jour : jours)
    {
        Serial.print(jour + " ");
        if (jourToWDay(jour) == wday)
        {
            jourOk = true;
        }
    }
    Serial.println();

    if (!jourOk)
    {
        Serial.println("DEBUG isActive: jour actuel non dans la liste des jours activés");
        return false;
    }

    int currentMinutes = now.tm_hour * 60 + now.tm_min;
    int debutMinutes = heureToMinutes(heureDebut);
    int finMinutes = heureToMinutes(heureFin);

    Serial.printf("DEBUG isActive: heureDebut='%s' -> %d minutes\n", heureDebut.c_str(), debutMinutes);
    Serial.printf("DEBUG isActive: heureFin='%s' -> %d minutes\n", heureFin.c_str(), finMinutes);
    Serial.printf("DEBUG isActive: currentMinutes=%d (%02d:%02d)\n", currentMinutes, now.tm_hour, now.tm_min);

    if (debutMinutes < 0 || finMinutes < 0)
    {
        Serial.println("DEBUG isActive: format heure invalide");
        return false;
    }

    bool active = (currentMinutes >= debutMinutes && currentMinutes < finMinutes);
    Serial.printf("DEBUG isActive: résultat final = %s\n", active ? "true" : "false");
    return active;
}

String HorlogeModule::getTimezone() const
{
    return timezone;
}

void HorlogeModule::appliquerTimezone() {
    Serial.print("🔧 Tente d'appliquer fuseau : '");
    Serial.print(timezone);
    Serial.println("'");

    if (timezone == "Indian/Antananarivo") {
        // UTC+3 => offset = 3*3600 secondes
        configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov"); 
        Serial.println("-> Fuseau configuré via configTime() pour Madagascar (UTC+3)");
    } else {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC par défaut
        Serial.println("-> Fuseau UTC par défaut");
    }

    delay(1000); // laisser le temps à configTime de se mettre à jour
    time_t now = time(nullptr);
    struct tm localTime;
    localtime_r(&now, &localTime);
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
    Serial.print("⏰ Heure locale après application TZ : ");
    Serial.println(buffer);
}
bool HorlogeModule::sauvegarderDansLittleFS(const char *path)
{
    if (!LittleFS.begin())
    {
        Serial.println("❌ LittleFS non monté ! Formatage en cours...");
        LittleFS.format();
        if (!LittleFS.begin())
        {
            Serial.println("❌ Échec du montage après formatage !");
            return false;
        }
    }

    FSInfo info;
    LittleFS.info(info);
    Serial.printf("FS total=%u, used=%u, free=%u\n", info.totalBytes, info.usedBytes, info.totalBytes - info.usedBytes);

    if (LittleFS.exists(path))
    {
        LittleFS.remove(path);
    }

    File file = LittleFS.open(path, "w");
    if (!file)
    {
        Serial.println("❌ Impossible d’ouvrir le fichier pour écriture");
        return false;
    }

    DynamicJsonDocument doc(4096);
    doc["enabled"] = enabled;
    doc["heureDebut"] = heureDebut;
    doc["heureFin"] = heureFin;
    doc["timezone"] = timezone;
    doc["state"] = state1 ? "ON" : "OFF"; // <-- Ajout de l'état actuel de l'éclairage
    JsonArray J = doc.createNestedArray("jours");
    for (auto &j : jours)
        J.add(j);

    size_t written = serializeJson(doc, file);
    file.close();

    Serial.printf("✅ JSON écrit : %u bytes, besoin estimé : %u bytes\n", written, measureJson(doc));

    return written > 0;
}
