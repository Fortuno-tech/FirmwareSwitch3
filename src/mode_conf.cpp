// mode_conf.cpp
#include <FS.h> // Pour lire SPIFFS
#include "mode_conf.h"

String getMode() {
  File file = SPIFFS.open("/mode.txt", "r");
  if (!file) return "online"; // Valeur par défaut
  String mode = file.readStringUntil('\n');
  mode.trim();
  file.close();
  return mode;
}
