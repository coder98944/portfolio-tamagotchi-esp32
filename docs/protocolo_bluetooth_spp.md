# 📡 Criterios de diseño: No uses JSON en placas con recursos limitados (ESP32, Arduino)

Si vienes del mundo del "desarrollo web" o "desarrollo de aplicaciones", probablemente tu primer instinto sea pensar: *"Hagamos una API y enviemos todo en un JSON"*. Esa fue mi primera impresión al iniciar la investigación de cómo conectar mi app de Android con mi Tamagotchi. 

En la ingeniería de hardware la eficiencia es un factor crítico. En este tipo de proyectos cada milisegundo de respuesta ("ms") cuenta para que una pantalla TFT (Thin Film Transistor) no se congele o directamente parpadee. Esto significa que "interpretar" largas cadenas de texto enviadas por red es un lujo que no nos podemos permitir. 

Entonces me topé con los llamados **Contratos de Datos** (Data Contracts), conceptos muy utilizados en el mundo del IoT (Internet of Things) donde la solución más primitiva suele ser la más elegante a nivel arquitectónico.

## ¿Qué protocolo usar? "BLE GATT" vs. "Classic SPP"

El panorama actual es el siguiente: BLE (Bluetooth Low Energy) es el estándar predeterminado para dispositivos de bajo consumo. Entonces, lo natural sería desplegar un servidor GATT (dispositivo que contiene los datos, ej. un sensor de temperatura) mediante un protocolo de comunicación que permite la transferencia de datos entre dispositivos usando un modelo jerárquico basado en servicios y características. 

Sin embargo, me encontré con un gran problema: **el exceso de protocolo ("overhead") hundiría indudablemente la tasa de Fotogramas Por Segundo (FPS) de mi pobre chip ESP32 con apenas 520KB de RAM.**

> [!NOTE]
> Al bloquear el microcontrolador (ESP32) procesando un complejo apretón de manos Bluetooth, su hilo principal (el *"Event-Loop"*) colapsaría. Esto significaría que si un usuario toca la pantalla para interactuar, el dispositivo estaría "sordo" y se perdería el toque táctil. Una pesadilla de UX. Esto lo comprobé de primera fuente cuando, al presionar el botón de dar de comer en la app, el Tamagotchi no reaccionaba en la pantalla.

**¿La solución?** Ir por la autopista con los peajes levantados. De igual forma, la solución más primitiva era la más eficiente. Así que decidí implementar el **Serial Port Profile (SPP)** del Bluetooth clásico usando la librería `BluetoothSerial`. Este protocolo transmite datos directamente de un búfer a otro sin hacer preguntas de metadatos, lo que lo hace ideal para proyectos con recursos limitados.

## El Arte de Enviar Un Solo Byte

En vez de mandar `{ "acción": "alimentar_mascota" }`, se envía **1 solo carácter** alfanumérico. La clase `NetworkMgr.cpp` del firmware actúa como un "portero" que acepta exclusivamente comandos binarios:

*   🍕 **Si el ESP32 lee `F` (Feed):** La FSM (Finite State Machine) del Tamagotchi se activa, se ejecuta la acción de alimentar, se dispara la animación de alegría y el decaimiento de hambre se reduce.
*   🎮 **Si el ESP32 lee `P` (Play):** Sube la felicidad instantáneamente, pero sacrificando un margen importante de energía.
*   💤 **Si el ESP32 lee `S` (Sleep):** Modifica el estado maestro del Tamagotchi a `SLEEP` y este entra en modo de reposo recuperando energía vital.

Cero parser JSON. Cero librerías pesadas bloqueantes. Cero latencia percibida por el usuario de Android. Todo fluyendo como el agua, sin interrupciones.

## La "Sala de Control" para QA (Trampas para mi propio Hardware)

Este es un secreto de desarrollo que te da control total sobre el flujo de pruebas (testing).

Piénsalo de esta manera: si logras codificar exitosamente una mascota virtual que muera de hambre en exactamente 6 horas del mundo real, ¿cómo haces las pruebas de diseño visual de Animación de Muerte? No puedes quedarte viendo el reloj durante 6 horas esperando a que la batería baje.

Fue indispensable implementar el **Cheat Mode** (Modo Trampa). 

> [!TIP]
> Un perfil junior típicamente se enfoca en el "Camino Feliz" del software del usuario final. Un ingeniero real reflexiona sobre cómo facilitar las pruebas de regresión, estrés y QA para su propio equipo de desarrollo.

Lo siguiente fue hacerle entender al código esta regla: *"Siempre lee el puerto Serial, pero si un comando comienza con la cabecera especial `#`, ignora a la App de Android y escúchame a mí"*. El firmware se pone automáticamente a la espera de recibir este formato exacto: `#<Hambre>,<Felicidad>,<Energía>\n`

De esta forma, si mañana quiero testear cómo reacciona mi animador matemático cuando el Tamagotchi esté al borde de la muerte, abro el Monitor Serial y le inyecto: `#5,100,50`. En el siguiente ciclo de reloj (`tick()`), asíncronamente y sin bloquear la UI del hardware, sobrescribo la memoria y salvo su vida mágicamente forzando la recuperación o probando lo inevitable.

Lo que aprendí al desarrollar esto es que a veces las metodologías web modernas nos impiden ver la solución más simple. Usar variables directas en Bytes e inyectar estados de memoria mediante un "cheat mode" garantizó toda la escalabilidad visual y de pruebas de este proyecto IoT.
