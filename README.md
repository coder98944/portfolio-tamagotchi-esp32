# Tamagotchi IoT (ESP32 + Android)

Proyecto educativo y de portafolio que integra firmware embebido en ESP32 con una app nativa Android para controlar una mascota virtual en tiempo real por Bluetooth Classic (SPP), con manejo de permisos según versión de Android para mejorar compatibilidad entre equipos antiguos y modernos.

![Tamagotchi IoT](docs/images/Tamagotchi_IoT.png)

## Qué problema resuelve

Este proyecto demuestra cómo diseñar una arquitectura IoT funcional cuando el hardware tiene recursos limitados (RAM, CPU y ancho de banda), evitando bloqueos en UI y manteniendo interacción fluida.

## Para quién está pensado

- Estudiantes que quieren aprender integración real entre firmware y app móvil.
- Desarrolladores que quieren practicar arquitectura embebida con ESP32.
- Equipos docentes o técnicos que necesitan un ejemplo end-to-end reproducible.

## Resultados técnicos del enfoque

- Control remoto de estados del Tamagotchi por Bluetooth Classic (SPP).
- Lógica desacoplada mediante FSM (Finite State Machine).
- Protocolo de comandos de 1 byte para minimizar latencia.
- Modo de pruebas (cheat mode) para QA rápido sin esperar ciclos largos.

## Arquitectura general

```mermaid
graph TD
    subgraph ESP32["Sistema embebido ESP32"]
        A["Bluetooth RFCOMM"] <-->|"Payload serial"| B["NetworkMgr"]
        B --> C{"Máquina de estados (FSM)"}
        C -->|"Cambio de estado"| D["Renderizado en framebuffer SRAM"]
        D -->|"Bus SPI"| E["Pantalla TFT XPT2046"]
        F["Interrupción táctil (IRQ)"] -->|"I2C/SPI"| D
        C --> G["LEDs de estado"]
    end

    subgraph Android["Cliente Android"]
        H["App nativa"] <-->|"Bluetooth Classic SPP"| A
    end
```

## Estructura del repositorio

- `Tamagotchi_sketch/`: firmware para ESP32 (Arduino).
  - Archivo principal: `Tamagotchi_sketch/Tamagotchi_sketch.ino`
  - Módulos clave: `Tamagotchi.cpp`, `NetworkMgr.cpp`
- `TamagotchiIoT/`: proyecto Android Studio.
  - App module: `TamagotchiIoT/app/`
  - Entrada principal: `MainActivity.java`
- `docs/`: documentación de decisiones técnicas.

## Requisitos

- ESP32
- Pantalla TFT compatible con controlador XPT2046 (según tu cableado)
- Arduino IDE (o entorno compatible)
- Android Studio
- Teléfono Android con Bluetooth

## Guía rápida (uso de los 2 proyectos)

1. **Cargar firmware en ESP32**
- Abre `Tamagotchi_sketch/Tamagotchi_sketch.ino` en Arduino IDE.
- Selecciona tu placa ESP32 y puerto serie.
- Compila y sube el firmware.

2. **Abrir app Android**
- Abre la carpeta `TamagotchiIoT/` en Android Studio.
- Sincroniza Gradle y ejecuta la app en dispositivo físico.

3. **Emparejar y conectar**
- Empareja el teléfono con el ESP32 por Bluetooth.
- Desde la app, conecta al dispositivo y envía comandos.
- La app aplica manejo de permisos por versión de Android (Android 12+ vs versiones anteriores).

4. **Comandos de control (protocolo simple)**
- `F`: alimentar
- `P`: jugar
- `S`: dormir

5. **Pruebas con cheat mode (QA)**
- Envía por serial una trama con formato:
- `#<hambre>,<felicidad>,<energía>\n`
- Ejemplo: `#5,100,50`

## Decisiones de ingeniería documentadas

- [Diseño de la máquina de estados](docs/logica_maquina_estados.md)
- [Optimización de memoria y framebuffer](docs/optimizacion_hardware_sram.md)
- [Protocolo Bluetooth SPP y contrato de datos](docs/protocolo_bluetooth_spp.md)

## Roadmap

- Modo de bajo consumo con `deepSleep` y wake-up por interrupción táctil.
- Mejoras de telemetría para medir latencia de comandos y ciclos de estado.

## Nota

Este repositorio está enfocado en aprendizaje aplicado y documentación técnica. La idea es que puedas modificar firmware y app para crear tu propia variante del Tamagotchi IoT.
