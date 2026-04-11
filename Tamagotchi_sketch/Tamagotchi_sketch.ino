/**
 * @file Tamagotchi_sketch.ino
 * @brief Hilo de ejecución principal para el Sistema Embebido Tamagotchi ESP32.
 * 
 * Características de arquitectura:
 * - Ruteo de Interrupciones por Hardware para UI Táctil (XPT2046).
 * - RFCOMM Bluetooth Serial bidireccional para recepción de comandos y QA.
 * - Framebuffer de renderizado asignado en SRAM (TFT_eSprite) para prevenir I/O flickering gráfico.
 */

#include "Tamagotchi.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include "src/NetworkMgr.h"

// --- HARDWARE DEFINITIONS ---
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define LED_RED 4
#define LED_GREEN 16
#define LED_BLUE 17

// --- CONFIGURACIÓN RETRO ---
#define RETRO_BG   0x9FD3 // Verde GameBoy
#define RETRO_INK  TFT_BLACK
#define RETRO_BTN_TXT TFT_WHITE
#define RETRO_BTN_BG  TFT_BLACK

// --- OBJETOS GLOBALES ---
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();
NetworkMgr network;
Tamagotchi myPet;

// Pre-asignación del Búfer en SRAM para aislar el bus SPI gráfico y evitar parpadeo visual.
// A una resolución de 100x100 (profundidad de color de 16 bits), consume ~20KB de SRAM nativa.
TFT_eSprite charSprite = TFT_eSprite(&tft);

// --- ASSETS RETRO (1-bit Bitmaps 16x16) ---
// 0 = Fondo, 1 = Tinta
const uint8_t retro_idle[] = {
  0x01, 0x80, // Fila 00: Botón (Squatchee)
  0x0F, 0xF0, // Fila 01: Copa
  0x7F, 0xF0, // Fila 02: Visera Ladeada
  0x10, 0x08, // Fila 03
  0x30, 0x0C, // Fila 04
  0x20, 0x04, // Fila 05
  0x60, 0x06, // Fila 06
  0xCC, 0x33, // Fila 07: Ojos
  0x8C, 0x31, // Fila 08
  0xC0, 0x03, // Fila 09
  0x41, 0x82, // Fila 10: Boca
  0x60, 0x06, // Fila 11
  0x3C, 0x3C, // Fila 12
  0x43, 0xC2, // Fila 13
  0x38, 0x1C, // Fila 14
  0x3C, 0x3C  // Fila 15
};

const uint8_t retro_blink[] = {
  0x01, 0x80, // Fila 00: Botón
  0x0F, 0xF0, // Fila 01: Copa
  0x7F, 0xF0, // Fila 02: Visera
  0x10, 0x08, // Fila 03
  0x30, 0x0C, // Fila 04
  0x20, 0x04, // Fila 05
  0x60, 0x06, // Fila 06
  0xC0, 0x03, // Fila 07: Párpado cerrado
  0x9C, 0x39, // Fila 08: Pestañas largas
  0xC0, 0x03, // Fila 09
  0x41, 0x82, // Fila 10: Boca
  0x60, 0x06, // Fila 11
  0x3C, 0x3C, // Fila 12
  0x43, 0xC2, // Fila 13
  0x38, 0x1C, // Fila 14
  0x3C, 0x3C  // Fila 15
};

// PIXEL: Boca abierta (Cuadrada/Circular) - CORREGIDO (Subido 3px)
const uint8_t retro_eat[] = {
  0x01, 0x80, // Fila 00: Botón
  0x0F, 0xF0, // Fila 01: Copa
  0x7F, 0xF0, // Fila 02: Visera
  0x10, 0x08, // Fila 03
  0x30, 0x0C, // Fila 04
  0x20, 0x04, // Fila 05
  0x60, 0x06, // Fila 06
  0xC0, 0x03, // Fila 07: Ojos cerrados
  0x8C, 0x31, // Fila 08: Pestañas CORTAS (2px)
  0xC7, 0xE3, // Fila 09: Boca Superior
  0x47, 0xE2, // Fila 10: Boca Inferior
  0x60, 0x06, // Fila 11
  0x3C, 0x3C, // Fila 12
  0x43, 0xC2, // Fila 13
  0x38, 0x1C, // Fila 14
  0x3C, 0x3C  // Fila 15
};

// PIXEL: Ojos Felices (^ ^) - CORREGIDO (Usando Blink)
const uint8_t retro_happy[] = {
  0x01, 0x80, // Fila 00
  0x0F, 0xF0, // Fila 01
  0x1F, 0xFE, // Fila 02
  0x10, 0x08, // Fila 03
  0xB0, 0x0D, // Fila 04
  0x20, 0x04, // Fila 05
  0x60, 0x06, // Fila 06
  0xC4, 0x23, // Fila 07
  0x8A, 0x51, // Fila 08
  0xC0, 0x03, // Fila 09
  0x64, 0x26, // Fila 10
  0x63, 0xC6, // Fila 11: Boca suavizada (63, C6)
  0x3C, 0x3C, // Fila 12
  0x03, 0xC0, // Fila 13
  0x38, 0x1C, // Fila 14
  0x3C, 0x3C  // Fila 15
};

// PIXEL: Dormir - CORREGIDO (Sin monocelda)
const uint8_t retro_sleep[] = {
  0x00, 0x00, // Fila 00
  0x01, 0x80, // Fila 01: Botón
  0x0F, 0xF0, // Fila 02: Copa
  0x7F, 0xF0, // Fila 03: Visera Idle
  0x10, 0x08, // Fila 04
  0x30, 0x0C, // Fila 05
  0x20, 0x04, // Fila 06
  0x60, 0x06, // Fila 07
  0xC0, 0x03, // Fila 08
  0x9C, 0x39, // Fila 09: Ojos cerrados
  0xC0, 0x03, // Fila 10
  0x60, 0x06, // Fila 11
  0x7F, 0xFE, // Fila 12: Cintura Squash
  0x38, 0x1C, // Fila 13: Pies Idle
  0x3C, 0x3C, // Fila 14: Pies Idle
  0x00, 0x00  // Fila 15
};

// PIXEL: Triste (Crítico)
const uint8_t retro_sad[] = {
  0x00, 0x00, // Fila 00
  0x01, 0x80, // Fila 01
  0x0F, 0xF0, // Fila 02
  0x1F, 0xF8, // Fila 03
  0x10, 0x08, // Fila 04
  0x30, 0x0C, // Fila 05
  0x20, 0x04, // Fila 06
  0x6A, 0x56, // Fila 07: Ojos tristes (caídos)
  0x80, 0x01, // Fila 08
  0xC0, 0x03, // Fila 09
  0x63, 0xC6, // Fila 10: Boca triste (Arco invertido)
  0x64, 0x26, // Fila 11
  0x3C, 0x3C, // Fila 12
  0x43, 0xC2, // Fila 13
  0x38, 0x1C, // Fila 14
  0x3C, 0x3C  // Fila 15
};

const uint8_t retro_dead[] = {
  0x00, 0x00, // Fila 00
  0x07, 0xF0, // Fila 01
  0x18, 0x8C, // Fila 02
  0x20, 0x04, // Fila 03
  0x20, 0x04, // Fila 04
  0x2E, 0xBA, // Fila 05
  0x2A, 0xAA, // Fila 06
  0x2E, 0xBA, // Fila 07
  0x2C, 0xA2, // Fila 08
  0x2A, 0xA2, // Fila 09
  0x20, 0x04, // Fila 10
  0x20, 0x84, // Fila 11
  0x3F, 0xFE, // Fila 12
  0x6F, 0xF6, // Fila 13
  0xCD, 0xB3, // Fila 14
  0x00, 0x00  // Fila 15
};

// --- VARIABLES DE ESTADO ---
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkInterval = 3000;
bool isBlinking = false;
unsigned long blinkStartTime = 0;

// Variables LED Globales
unsigned long lastLedBlink = 0;
bool ledBlinkState = false;
int strobeIndex = 0; // Para el efecto sirena

// Variables para evitar redibujado de texto innecesario
int lastHunger = -1;
int lastEnergy = -1;
int lastHappiness = -1;

// Algoritmo de escalado espacial ("Upscaling") in-memory para cargar bitmaps de 1-bit de 16x16 píxeles.
// Reduce drásticamente la huella de memoria Flash en comparación al alojamiento de sprites completos a color (RGB565).
void drawBitmapInSprite(const uint8_t* bitmap, int x, int y, int scale) {
    for (int row = 0; row < 16; row++) {
        uint16_t rowBits = (bitmap[row * 2] << 8) | bitmap[row * 2 + 1];
        for (int col = 0; col < 16; col++) {
            if ((rowBits >> (15 - col)) & 0x01) {
                charSprite.fillRect(x + col * scale, y + row * scale, scale, scale, RETRO_INK);
            }
        }
    }
}

// PIXEL: UI Estática (Fondo y Botones)
void drawStaticUI() {
    tft.fillScreen(RETRO_BG);
    
    // Línea separadora superior (Ajustada a 50px para fuente pequeña apilada)
    tft.drawFastHLine(0, 50, 320, RETRO_INK);
    tft.drawFastHLine(0, 51, 320, RETRO_INK);
    
    // Botones (Abajo)
    int btnY = 190;
    int btnH = 40;
    int btnW = 90;
    int gap = 10;
    int startX = 15;
    
    // Botón COMER
    tft.fillRect(startX, btnY, btnW, btnH, RETRO_BTN_BG);
    tft.setTextColor(RETRO_BTN_TXT);
    tft.drawCentreString("COMER", startX + btnW/2, btnY + 12, 2);
    
    // Botón JUGAR
    tft.fillRect(startX + btnW + gap, btnY, btnW, btnH, RETRO_BTN_BG);
    tft.drawCentreString("JUGAR", startX + btnW + gap + btnW/2, btnY + 12, 2);
    
    // Botón DORMIR
    tft.fillRect(startX + (btnW + gap)*2, btnY, btnW, btnH, RETRO_BTN_BG);
    tft.drawCentreString("DORMIR", startX + (btnW + gap)*2 + btnW/2, btnY + 12, 2);
}

// Mecanismo "Lazy Render / Caching": Exige redibujar las primitivas de texto EXCLUSIVAMENTE ante mutaciones
// provenientes de la Máquina de Estados (FSM), previniendo un consumo estéril de ciclos de CPU en cada "tick".
void updateStatsUI() {
    int h = myPet.getHunger();
    int e = myPet.getEnergy();
    int ha = myPet.getHappiness();
    
    if (h != lastHunger || e != lastEnergy || ha != lastHappiness) {
        // Borrar área de texto (50px)
        tft.fillRect(0, 0, 320, 50, RETRO_BG);
        
        tft.setTextColor(RETRO_INK);
        
        // 1. Etiquetas (Fuente Pequeña - Size 1)
        tft.setTextSize(1);
        tft.drawCentreString("HAMBRE", 53, 5, 1);
        tft.drawCentreString("FELICIDAD", 160, 5, 1); // Reordenado (Centro)
        tft.drawCentreString("ENERGIA", 266, 5, 1);   // Reordenado (Derecha)
        
        // 2. Valores (Fuente Grande - Size 2)
        tft.setTextSize(2);
        tft.drawCentreString(String(h) + "%", 53, 20, 1);
        tft.drawCentreString(String(ha) + "%", 160, 20, 1); // Valor Felicidad
        tft.drawCentreString(String(e) + "%", 266, 20, 1);  // Valor Energía
        
        lastHunger = h;
        lastEnergy = e;
        lastHappiness = ha;
    }
}

// Helper para Lógica Invertida (Active Low)
void setLedColor(bool r, bool g, bool b) {
    digitalWrite(LED_RED, r ? LOW : HIGH);
    digitalWrite(LED_GREEN, g ? LOW : HIGH);
    digitalWrite(LED_BLUE, b ? LOW : HIGH);
}

void updateLEDs(State state) {
    // Obtener stats actuales
    int h = myPet.getHunger();
    int e = myPet.getEnergy();
    int ha = myPet.getHappiness();
    
    unsigned long currentMillis = millis();

    // --- JERARQUÍA DE NECESIDADES ---

    // 0. MUERTE -> ROJO FIJO
    if (state == DEAD) {
        setLedColor(true, false, false); // Rojo
    }
    // 1. DORMIR (Estado Activo) -> AZUL FIJO
    else if (state == SLEEP) {
        setLedColor(false, false, true); // Azul
    }
    // 2. CRÍTICO (TODOS < 10%) -> SIRENA RGB RÁPIDA
    else if (h < 10 && ha < 10 && e < 10) {
        // Strobe cada 100ms
        if (currentMillis - lastLedBlink > 100) {
            lastLedBlink = currentMillis;
            strobeIndex = (strobeIndex + 1) % 3;
            if (strobeIndex == 0) setLedColor(true, false, false); // Rojo
            else if (strobeIndex == 1) setLedColor(false, false, true); // Azul
            else setLedColor(true, true, false); // Amarillo/Verde
        }
    }
    // 3. ADVERTENCIA (< 40%) -> PRIORIDAD DINÁMICA (El más bajo manda)
    else if (h < 40 || ha < 40 || e < 40) {
        // Determinar cuál es el stat más bajo (más urgente)
        int minStat = min(h, min(ha, e));
        
        // Parpadeo Lento (500ms)
        if (currentMillis - lastLedBlink > 500) { 
            lastLedBlink = currentMillis; 
            ledBlinkState = !ledBlinkState; 
        }

        if (ledBlinkState) {
            if (minStat == h) {
                setLedColor(true, false, false); // Rojo (Hambre es el peor)
            } else if (minStat == ha) {
                setLedColor(true, false, true);  // Violeta (Felicidad es el peor)
            } else {
                setLedColor(false, false, true); // Azul (Energía es el peor)
            }
        } else {
            setLedColor(false, false, false); // Apagado
        }
    }
    // 4. ESTADO NORMAL (TODOS >= 40%) -> VERDE FIJO
    else {
        setLedColor(false, true, false); // Verde
    }
}

// Pipeline gráfico de multiplexación asíncrono y carente de bloqueos (Non-blocking)
void drawCharacterLoop() {
    // 1. Limpiar Sprite (Fondo Verde)
    charSprite.fillSprite(RETRO_BG);
    
    // 2. Lógica de Pestañeo
    unsigned long currentMillis = millis();
    if (!isBlinking && currentMillis - lastBlinkTime > nextBlinkInterval) {
        isBlinking = true;
        blinkStartTime = currentMillis;
    }
    if (isBlinking && currentMillis - blinkStartTime > 200) {
        isBlinking = false;
        lastBlinkTime = currentMillis;
        nextBlinkInterval = random(2000, 4000); // Próximo pestañeo aleatorio
    }
    
    // 3. Seleccionar Bitmap
    const uint8_t* currentBitmap = retro_idle;
    State state = myPet.getState();
    
    // Chequear estado crítico para la cara triste (Coincide con Sirena LED)
    bool isCritical = (myPet.getHunger() < 10 && myPet.getHappiness() < 10 && myPet.getEnergy() < 10);

    if (state == DEAD) {
        currentBitmap = retro_dead;
    } else if (state == SLEEP) {
        currentBitmap = retro_sleep;
    } else if (isCritical) {
        currentBitmap = retro_sad; // Cara Triste en Crítico
    } else {
        // HAPPY / NORMAL / WARNING
        if (isBlinking) currentBitmap = retro_blink;
        else currentBitmap = retro_idle;
    }
    
    // 4. Dibujar en Sprite (Escalado x5 para entrar en 100x100 -> 16*5 = 80px)
    // Centrado en 100x100: (100-80)/2 = 10
    drawBitmapInSprite(currentBitmap, 10, 10, 5);
    
    // Zzz si duerme (COORDENADAS AJUSTADAS)
    if (state == SLEEP) {
        charSprite.setTextColor(RETRO_INK);
        charSprite.drawString("Zzz...", 70, 0, 2); // Flotando al lado
    }
    
    // 5. Push Sprite a Pantalla (Centro del área media)
    // Área media: 50 a 190 (Alto 140). Centro Y = 120.
    // Sprite 100x100. Pos: 110, 70
    charSprite.pushSprite(110, 70);
}

// ANIMACIÓN: COMER (Masticar)
void animateEating() {
    for(int i=0; i<3; i++) {
        // Boca Abierta
        charSprite.fillSprite(RETRO_BG);
        drawBitmapInSprite(retro_eat, 10, 10, 5);
        
        // Globo de texto (Simulado)
        charSprite.setTextColor(RETRO_INK);
        charSprite.drawString("Yum!", 70, 5, 2);
        
        charSprite.pushSprite(110, 70);
        delay(200);
        
        // Boca Cerrada
        charSprite.fillSprite(RETRO_BG);
        drawBitmapInSprite(retro_idle, 10, 10, 5);
        
        // Globo de texto (Mantener)
        charSprite.drawString("Yum!", 70, 5, 2);
        
        charSprite.pushSprite(110, 70);
        delay(200);
    }
}

// ANIMACIÓN: JUGAR (Feliz)
void animatePlaying() {
    setLedColor(true, false, true); // LED VIOLETA (Feedback Visual)
    
    charSprite.fillSprite(RETRO_BG);
    drawBitmapInSprite(retro_happy, 10, 10, 5);
    
    // Globo de texto (Estilo Yum/Zzz)
    charSprite.setTextColor(RETRO_INK);
    charSprite.drawString("Yey!", 70, 5, 2);
    
    charSprite.pushSprite(110, 70);
    
    delay(1000); // Mantener cara feliz 1 segundo
}

// Animaciones bloqueantes breves para feedback inmediato
void showActionFeedback(const uint8_t* bitmap) {
    charSprite.fillSprite(RETRO_BG);
    drawBitmapInSprite(bitmap, 10, 10, 5);
    charSprite.pushSprite(110, 70);
    delay(500);
}

// --- SETUP ---
void setup() {
    Serial.begin(115200);

    // Init LEDs (Fix de Conexión)
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    // Apagar al inicio (Lógica Invertida: HIGH = OFF)
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_BLUE, HIGH);
    
    // Init Hardware
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
    tft.init();
    tft.setRotation(1);
    
    // Init Sprite
    charSprite.createSprite(100, 100);
    charSprite.setColorDepth(16);
    
    // Dibujar UI Estática
    drawStaticUI();
    
    // Init Bluetooth
    network.begin();
    
    // Init Lógica
    myPet.begin();
    
    // Siembra del motor lógico pseudo-aleatorio
    randomSeed(analogRead(0));
}

// --- LOOP ---
void loop() {
    // 1. Actualizar Lógica
    myPet.update();
    
    // Actualizar Luces según estado (Fix de Conexión)
    updateLEDs(myPet.getState());
    
    // 2. Actualizar Stats UI
    updateStatsUI();
    
    // 3. Dibujar Personaje (Buffer)
    drawCharacterLoop();
    
    // 4. Interceptador de rutinas de Panel Táctil (Hardware IRQ handling)
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        // Mapeo de coordenadas (ajustar según calibración, asumiendo standard 0-4095)
        // En rotación 1: x va de 0 a 320 aprox, y de 0 a 240.
        // NOTA: XPT2046 devuelve valores crudos (0-4095). Necesitamos map.
        // Calibración empírica aproximada para pantalla 320x240
        int touchX = map(p.x, 200, 3800, 0, 320);
        int touchY = map(p.y, 200, 3800, 0, 240);
        
        // Zona Botones (Y > 180)
        if (touchY > 180 && myPet.getState() != DEAD) {
            if (touchX < 106) { // Botón 1: COMER
                myPet.feed();
                animateEating(); // PIXEL: Animación Masticar
            } else if (touchX < 213) { // Botón 2: JUGAR
                myPet.play();
                animatePlaying(); // PIXEL: Animación Feliz
            } else { // Botón 3: DORMIR
                myPet.sleep();
                showActionFeedback(retro_sleep);
            }
            // Debounce simple
            delay(300);
        }
    }
    
    
    // 5. Polling asíncrono para el búfer serial (RFCOMM Bluetooth)
    // Primero, evaluamos el "Cheat Mode": Vector de telemetría inyectado manual por puerto serie (`#X,X,X`)
    int cheatHunger, cheatHappiness, cheatEnergy;
    if (network.checkCheatCommand(cheatHunger, cheatHappiness, cheatEnergy)) {
        // Aplicar los valores directamente al Tamagotchi
        myPet.setStats(cheatHunger, cheatHappiness, cheatEnergy);
        showActionFeedback(retro_happy); // Mostrar feedback visual
    } else {
        // Si no es comando cheat, verificar comandos normales
        char remoteAction = network.checkRemoteInput();
        if (remoteAction != 0 && myPet.getState() != DEAD) {
            if (remoteAction == 'F') { myPet.feed(); animateEating(); }
            if (remoteAction == 'P') { myPet.play(); animatePlaying(); }
            if (remoteAction == 'S') { myPet.sleep(); showActionFeedback(retro_sleep); }
        }
    }
    
    delay(20); // Estabilidad
}