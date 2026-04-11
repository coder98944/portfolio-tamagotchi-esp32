#ifndef TAMAGOTCHI_H
#define TAMAGOTCHI_H

#include <Arduino.h>

// Definición de Estados del Tamagotchi
enum State {
    IDLE,
    HUNGRY,
    SLEEP,
    HAPPY,
    DEAD // Nuevo Estado: Muerte
};

// Clase Tamagotchi
class Tamagotchi {
private:
    int _hunger;
    int _happiness;
    int _energy;
    bool _isSleeping; // Estado manual de sueño

    // Variables de Tiempo
    unsigned long _lastTick;      // Marca de tiempo del último ciclo de actualización
    const unsigned long _tickInterval = 5000; // Intervalo para bajar stats (ej: cada 5 seg)

    // Umbrales para cambio de estado automático
    const int THRESHOLD_HUNGRY = 30; // Si _hunger < 30 -> Estado HUNGRY
    const int THRESHOLD_SLEEP = 20;  // Si _energy < 20 -> Estado SLEEP (forzado o sugerido)

public:
    // Constructor
    Tamagotchi();

    // Inicialización
    void begin();

    // Ciclo Principal (Llamar en loop, sin delay)
    void update();

    // Acciones del Usuario
    void feed(); // Aumenta _hunger (+20)
    void play(); // Aumenta _happiness (+10), reduce _energy
    void sleep(); // Recupera _energy, bloquea otras acciones mientras duerme?

    // Getters para la UI y Lógica
    int getHunger() const;
    int getHappiness() const;
    int getEnergy() const;
    State getState() const;

    // Setters / Debug (Establecer valores)
    void setStats(int h, int ha, int e);
};

#endif
