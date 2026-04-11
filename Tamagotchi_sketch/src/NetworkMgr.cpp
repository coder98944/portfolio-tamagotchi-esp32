#include "NetworkMgr.h"

// Clase NetworkMgr
void NetworkMgr::begin() {
    SerialBT.begin("Tamagotchi_IoT");
}
// Verificar entrada remota
char NetworkMgr::checkRemoteInput() {
    if (SerialBT.available()) {
        return (char)SerialBT.read();
    }
    return 0;
}
// Verificar comando cheat
bool NetworkMgr::checkCheatCommand(int &hunger, int &happiness, int &energy) {
    if (SerialBT.available()) {
        // Verificar PRIMERO si es un comando cheat sin consumir el dato
        char firstChar = SerialBT.peek(); // peek NO consume el carácter
        // Si es un comando cheat
        if (firstChar == '#') {
            // Solo ahora leer la línea completa
            String line = SerialBT.readStringUntil('\n');
            line.trim();
            
            // Remover el '#'
            line.remove(0, 1);
            
            // Parsear los valores: H,F,E
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            // Si es un comando cheat valido
            if (firstComma > 0 && secondComma > firstComma) {
                hunger = line.substring(0, firstComma).toInt();
                happiness = line.substring(firstComma + 1, secondComma).toInt();
                energy = line.substring(secondComma + 1).toInt();
                return true;
            }
        }
        // Si no es '#', no hacer nada y dejar el carácter en el buffer
    }
    return false;
}
