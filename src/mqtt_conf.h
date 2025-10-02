#ifndef MQTT_CONF_H
#define MQTT_CONF_H

#include <Arduino.h>
#include <PubSubClient.h>  // <-- ajouter cette ligne
extern bool state1;
extern int outputPin1;
extern bool state2;
extern int outputPin2;
extern bool state3;
extern int outputPin3;
extern PubSubClient client;
void startAPMode();
void handleWiFiAndMQTT();
void setupServer();
void callback(char *topic, byte *payload, unsigned int length);
void checkForUpdates(String firmware_url);
void reconnecter();
void setupCallBack();
void nouvelEtat();
void outputPin();
void inputCheck();
void setupPin();


#endif
