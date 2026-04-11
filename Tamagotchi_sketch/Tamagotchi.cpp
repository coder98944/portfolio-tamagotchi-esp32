#include "Tamagotchi.h"

Tamagotchi::Tamagotchi() {
    // Valores iniciales equilibrados (Lex autoriza cambio para UX)
    _hunger = 0; // Test Muerte (Hambre Crítica)
    _happiness = 5;
    _energy = 15;
    _isSleeping = false;
    _lastTick = 0;
}

void Tamagotchi::begin() {
    _lastTick = millis();
}

void Tamagotchi::feed() {
    if (getState() == DEAD) return; // Bloqueo por Muerte
    _isSleeping = false; // Despertar si come
    // Regla: Comer aumenta +20 Hambre y +5 Energía
    _hunger += 20;
    _energy += 5;
    
    // Regla Crítica (Lex): Clamping a 100
    if (_hunger > 100) _hunger = 100;
    if (_energy > 100) _energy = 100;
}

void Tamagotchi::play() {
    if (getState() == DEAD) return; // Bloqueo por Muerte
    // Jugar aumenta felicidad pero da hambre
    _happiness += 10;
    _hunger -= 5; // Antes era energía, ahora hambre

    // CLAMPING (Límites)
    if (_happiness > 100) _happiness = 100;
    if (_hunger < 0) _hunger = 0;
    _isSleeping = false; // Despertar si juega
}

void Tamagotchi::sleep() {
    if (getState() == DEAD) return; // Bloqueo por Muerte
    // Toggle manual de sueño
    _isSleeping = !_isSleeping;
}

void Tamagotchi::update() {
    unsigned long currentMillis = millis();

    // FSM Tick - Decaimiento de métricas asíncrono sobre intervalos fijos
    if (currentMillis - _lastTick >= _tickInterval) {
        _lastTick = currentMillis;

        // Decaimiento natural de stats
        _hunger--;
        
        // Determinar si está durmiendo (Solo Manual para Test Muerte)
        bool currentlySleeping = _isSleeping; // || (_energy < 20);

        if (!currentlySleeping) {
            _energy--;     // Solo gasta energía si está despierto
            _happiness--;  // Felicidad decae despierto
        } else {
            // RECARGA DE ENERGÍA (Fix Crítico)
            _energy += 5;  // Recarga rápida al dormir
            if (_energy > 100) {
                _energy = 100;
                _isSleeping = false; // Despertar automático al estar lleno
            }
        }

        // Clamping inferior
        if (_hunger < 0) _hunger = 0;
        if (_energy < 0) _energy = 0;
        if (_happiness < 0) _happiness = 0;
    }
}

State Tamagotchi::getState() const {
    // Máquina de Estados Finita (FSM) Evaluador Central
    
    // Prioridad 0: ESTADO TERMINAL (Si toda la telemetría se reduce a cero)
    if (_hunger == 0 && _energy == 0 && _happiness == 0) {
        return DEAD;
    }

    // Prioridad 1: Sueño Manual (Automático deshabilitado para Test Muerte)
    if (_isSleeping) { // || _energy < 20) {
        return SLEEP;
    }
    
    // Prioridad 2: Necesidad Fisiológica (Hambre)
    // Nota: _hunger es Saciedad. Bajo valor = Hambriento.
    if (_hunger < 30) {
        return HUNGRY;
    }

    // Prioridad 3: Estado por defecto
    // Podríamos agregar lógica para "TRISTE" si happiness es bajo, 
    // pero por ahora por defecto a HAPPY o NORMAL
    return HAPPY;
}

// Getters (Obtención de datos)
int Tamagotchi::getHunger() const { return _hunger; }
int Tamagotchi::getHappiness() const { return _happiness; }
int Tamagotchi::getEnergy() const { return _energy; }

// Debug / Testing
void Tamagotchi::setStats(int h, int ha, int e) {
    _hunger = h;
    _happiness = ha;
    _energy = e;
}
