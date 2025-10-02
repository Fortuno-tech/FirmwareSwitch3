#ifndef HORLOGEMODULE_H
#define HORLOGEMODULE_H

#include <Arduino.h>
#include <vector>
#include <time.h>

class HorlogeModule
{
private:
    bool enabled;
    String heureDebut;
    String heureFin;
    String timezone;
    std::vector<String> jours;
    bool state1; // ton état interne

public:
    HorlogeModule();

    // Getter pour enabled (pour accéder depuis l'extérieur)
    bool isEnabled() const;
    bool getState() const { return state1; }
    // Méthodes principales
    void onMessage(const char *payload, size_t length);
    bool chargerDepuisLittleFS(const char *path);
    void parseJson(const char *payload, size_t length);
    bool sauvegarderDansLittleFS(const char *path);
    void appliquerTimezone();

    // Calculs et vérifications
    int jourToWDay(const String &jour) const;
    int heureToMinutes(const String &h) const;
    bool isActive(const struct tm &now) const;

    // Getter timezone
    String getTimezone() const;

    // (Optionnel) Setter enabled si besoin (à adapter selon logique)
    void setEnabled(bool value);

    // Autres getters/setters si nécessaire
    const String &getHeureDebut() const { return heureDebut; }
    const String &getHeureFin() const { return heureFin; }
    const std::vector<String> &getJours() const { return jours; }

    void setHeureDebut(const String &hd) { heureDebut = hd; }
    void setHeureFin(const String &hf) { heureFin = hf; }
    void setTimezone(const String &tz) { timezone = tz; }
    void setJours(const std::vector<String> &j) { jours = j; }
};

#endif // HORLOGEMODULE_H
