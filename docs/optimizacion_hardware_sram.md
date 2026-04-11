# 🔌 El truco de la RAM (SRAM) que salvó la pantalla del ESP32

Este documento describe la topología física del sistema y la técnica de arquitectura de "Memoria a Corto Plazo" que implementé para que las animaciones del Tamagotchi fueran impecables.

## Entendiendo el "Secuestro" de Memoria

El ESP32 (el microcontrolador físico principal) viene de fábrica con exactamente **520 KB de SRAM** total disponible. Esa microscópica porción es toda la memoria "a corto plazo" (RAM) que tiene el chip para hacer absolutamente todo: mantener viva la conexión inalámbrica de Bluetooth, procesar la de lógica de mi programa y calcular las alertas.
Aquí es donde entra la ingeniería de software: para evadir el parpadeo y la lentitud extrema de la pantalla TFT, apliqué una técnica de renderizado invisible. Usando la clase de C++ `TFT_eSprite`, "secuestré" de manera intencional y calculada un espacio del tamaño exacto de **20 KB** de toda esa RAM disponible.

> [!TIP]
> A este espacio reservado se le denomina **Framebuffer**. Al separar esos 20 KB de mis 520 KB totales, el cerebro del ESP32 dibuja toda la cara de la mascota y sus datos "en las sombras" dentro de su propia RAM. Una vez que el recuadro de la imagen está listo internamente, empuja el bloque a la pantalla. El resultado es un salto monstruoso en el rendimiento y FPS del hardware, sin saturar al chip.

## Conexiones y Pines: Dejando de lado la Ineficiencia

Para que la mascota reaccione a los toques del usuario en pantalla, las conexiones debian ser las siguientes:

| Periférico        | Pin Físico (ESP32) | Propósito en la Arquitectura             |
| :---------------- | :----------------- | :--------------------------------------- |
| **XPT2046 CS**    | `GPIO 33`          | Switch encendido/apagado interno del Panel Táctil. |
| **XPT2046 IRQ**   | `GPIO 36`          | **Interrupción de Hardware:** Este pin es como un "botón del pánico". Le inyecta corriente directa a la CPU solo cuando el usuario toca la pantalla. Esto es vital, ya que evita que el programa pierda tiempo en cada ciclo "preguntando" "iterativamente" si alguien lo tocó (ahorrando tiempo valioso). |
| **LED STATUS_R**  | `GPIO 4`           | Led Físico de Estado: **Mascota en Crisis (Hambre).** |
| **LED STATUS_G**  | `GPIO 16`          | Led Físico de Estado: **Mascota Saludable y Feliz.** |
| **LED STATUS_B**  | `GPIO 17`          | Led Físico de Estado: **Mascota Durmiendo.** |

> [!NOTE]
> Respecto a los buses de red compartida (SPI): El ESP32, por dentro, tiene pequeños circuitos físicos especializados y separados del cerebro principal (periféricos SPI). Al usar el modo "Nativo", la librería no "trabaja", simplemente toma todo el bloque de tu Framebuffer (~20KB) y se lo lanza a este circuito especializado. Este circuito, operando de forma autónoma, inyecta los datos directamente a la pantalla, dejando al cerebro principal 100% libre para seguir pensando en Bluetooth o en la Máquina de Estados. Aceleración por hardware en los videojuegos!🎮
