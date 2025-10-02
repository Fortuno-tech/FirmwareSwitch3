#include "otaOffline.h"
#include <ArduinoOTA.h>


void initOTA() {
  String macAddress = WiFi.macAddress();
  macAddress.replace(":", "");
 ArduinoOTA.setHostname(("Smartswitch" + macAddress.substring(0, 4)).c_str());

  ArduinoOTA.onStart([]() {
    Serial.println("\nDébut de la mise à jour");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nMise à jour terminée");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progression : %u%%\r", (progress * 100) / total);
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Erreur [%u] : ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Erreur d'authentification");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Erreur de démarrage");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Erreur de connexion");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Erreur de réception");
    else if (error == OTA_END_ERROR) Serial.println("Erreur de finalisation");
  });

  ArduinoOTA.begin();
  Serial.println("OTA prêt");
}