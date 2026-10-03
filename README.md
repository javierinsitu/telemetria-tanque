# Telemetría hidráulica — ESP32-C6 Zero

Firmware modular ESP-IDF puro para ADS1115/HY-5000, YF-B6 y Zigbee Router.
Sin Arduino ni ESPHome. Versión de protocolo/firmware **0.2.1**.

Código integrado, compilado y flasheado en el ESP32-C6 conectado. Se comprobaron
arranque, lecturas ADC, pruebas de cálculo y actualización de atributos ZCL.
El SDK también restauró el estado de red tras un reinicio (`unido=1`). Falta
confirmar reporting y entidades en Zigbee2MQTT/Home Assistant, exactitud hidráulica
y funcionamiento prolongado. Ver [VALIDATION.md](docs/VALIDATION.md) y la lista
priorizada de [TODOs](TODO.md).

## Características

| Función | Implementación actual |
|---|---|
| Nivel | HY-5000 de 4–20 mA con shunt de 150 Ω y ADS1115; metros y porcentaje, calibración e IIR |
| Diagnóstico eléctrico | Voltaje y corriente del lazo, invalidación de lecturas y estado de nivel |
| Caudal | YF-B6 con PCNT, filtro de pulsos y extensión a 64 bits; pendiente/intercepto configurables |
| Volumen acumulado | Integración del caudal sin filtro; conservación tras reinicio y reset explícito |
| Muestreo y radio | Muestreo de 1 s, última muestra en RAM; reporting mínimo 30 s por atributo, heartbeat 600 s |
| Persistencia | NVS versionada, checkpoints de volumen de 10 min y configuración con debounce de 30 s |
| Zigbee | Router, radio nativa C6, identidad, incorporación con backoff y recuperación de estado de red |
| Zigbee2MQTT | Conversor externo ESM: ocho entidades, catorce parámetros y acción de reset |
| Modularidad | Componentes independientes para ADC, nivel, caudal, Zigbee, NVS, configuración y diagnóstico |
| Verificación | Compilación y flasheo reales, pruebas de cálculo en placa y pruebas de conversor con ZHC |

El caudal está deshabilitado de fábrica hasta comprobar su divisor eléctrico.
La implementación de una función no implica que su calibración física o su
integración con Home Assistant hayan sido aceptadas; esos ensayos están en TODO.md.

## Versiones

- ESP-IDF **6.0.2** y `espressif/esp-zigbee-lib` **2.0.3**, fijados en manifiesto
  y `dependencies.lock`; APIs nativas v2.x `ezb_*`, sin compatibilidad v1.x.
- Compilación/enlazado y arranque de radio nativa comprobados en este C6, revisión
  v0.2. La [guía oficial de Espressif](https://docs.espressif.com/projects/esp-zigbee-sdk/en/latest/esp32c6/developing.html)
  recomienda IDF 5.5.4; este proyecto usa la versión 6.0.2 instalada, admitida
  por el manifiesto SDK (>=5.0) y verificada aquí. No se generaliza a otros targets.
- Conversor ESM validado mediante `prepareDefinition` de
  zigbee-herdsman-converters **26.108.1** con su zigbee-herdsman real.
  Falta probar la versión de Zigbee2MQTT instalada en Home Assistant.
- Flash real de esta placa: **8 MB**. La partición de aplicación sigue en 1 MB;
  se conserva la ubicación/tamaño original de NVS para no borrar credenciales.

## Conexiones

| Componente | Conexión |
|---|---|
| HY-5000 rojo | +12 V de fuente externa |
| HY-5000 negro | Extremo superior del shunt 150 Ω y ADS1115 A0 |
| Shunt extremo inferior | GND común |
| ADS1115 VDD / GND | ESP32 3V3 / GND común |
| ADS1115 SDA / SCL | GPIO3 / GPIO2 |
| ADS1115 ADDR | GND (dirección 0x48) |
| SDA y SCL | Pullups externas a 3,3 V, típicamente 4,7 kΩ; comprobar las del módulo |
| ADS1115 ALERT/RDY | Sin conectar; adquisición mediante polling del registro OS |
| YF-B6 alimentación | 12 V / GND común |
| YF-B6 señal | R1 30 kΩ hacia GPIO4; R2 10 kΩ de GPIO4 a GND; comprobar antes de conectar |

ESP32 se alimenta por USB o por su entrada regulada según la placa, nunca con
12 V en 3V3. Colocar desacoplo 100 nF cerca del ADS1115. El shunt disipa 0,06 W
a 20 mA: usar resistencia de precisión, al menos 0,25 W, y considerar sobrecorrientes.
Comprobar que el sensor concreto es de lazo de dos hilos antes de alimentarlo.

El divisor entrega nominalmente Vseñal/4: 12 V producen 3 V y 13,2 V producen
3,3 V. No garantiza protección ante sobretensión ni que la salida del sensor
sea compatible: medir tensión alta/baja, transitorios y forma de onda con el
sensor alimentado y GPIO4 desconectado. Validar tolerancias, alimentación máxima
y umbrales lógicos del C6 antes de conectarlo; usar adaptación/protección adecuada
si no se cumplen. GPIO4 se configura al habilitar `flow_enabled=1`.

## Compilar, flashear y observar

```sh
cd /home/javier/esp32/telemetria-tanque
. /home/javier/.local/share/esp-idf-v6.0.2/export.sh
idf.py -B /tmp/telemetria-tanque-integrated-build build
idf.py -B /tmp/telemetria-tanque-integrated-build -p /dev/ttyACM0 flash
idf.py -B /tmp/telemetria-tanque-integrated-build -p /dev/ttyACM0 monitor
```

Target `esp32c6` en defaults. En una configuración previa para otro target usar
`idf.py set-target esp32c6`. ESP-IDF Component Manager descarga el SDK fijado.
Salir del monitor con Ctrl+]. Flashear sin `erase-flash`: conserva NVS y volumen.
Menú `idf.py -B /tmp/telemetria-tanque-integrated-build menuconfig`:
dirección ADS1115, intervalo inicial y pruebas de cálculo opcionales al arrancar.
La configuración persistida prevalece sobre defaults de compilación.

## Incorporación a Zigbee2MQTT y Home Assistant

1. Instalar [telemetria_tanque.mjs](zigbee2mqtt/telemetria_tanque.mjs), siguiendo
   [zigbee2mqtt/README.md](zigbee2mqtt/README.md), antes de incorporar el dispositivo.
2. Abrir **Permitir incorporación** en Zigbee2MQTT. El firmware busca canales
   11–26 con backoff de 5 s hasta 5 min y jitter; dejarlo alimentado durante la
   búsqueda. No requiere pulsar un botón. El rol de red es Router.
3. Verificar identidad `MILANGAS / ESP32C6_HYDRAULIC_1`, interview y Configure
   Reporting. El log debe mostrar `Unido PAN=... canal=...`.
4. Si se incorporó antes del conversor, cargarlo/reiniciar Zigbee2MQTT y ejecutar
   **Reconfigure**. No borrar NVS ni exigir reemparejado como primer recurso.
5. Con la integración MQTT y descubrimiento HA activos, se crean seis sensores
   numéricos y dos estados. Leer las entidades desde Zigbee2MQTT y observar MQTT.

No hay transmisión de mediciones periódica manual adicional: el coordinador
configura reporting estándar. Cache por defecto cada 30 s, reporting mínimo
30 s, máximo 600 s, con umbrales por magnitud. `publication_s` controla aceptación
para cache; el coordinador controla los intervalos de reporting por aire.
Actualizar cache no es confirmación de transmisión ni recepción en HA.
Se conserva la última muestra en RAM y el volumen integrado, no un historial de
cada segundo. Los intervalos guardados de 5–29 s se migran a 30 s conservando
calibración y volumen. El conversor limita reporting a una actualización por
atributo cada 30 s; varios endpoints pueden emitir cerca del mismo instante.
Actualizar el conversor y ejecutar **Reconfigure** para aplicar ese mínimo a un
dispositivo ya incorporado. No necesita reemparejarse.

## Sensores y calibración

ADS1115: A0/GND, ±4,096 V, single-shot, 128 SPS, comparador deshabilitado,
configuración `0xC383`, MSB primero, 125 µV/cuenta. Se comprueba configuración
y OS listo antes de leer. Referencia: [TI ADS1115](https://www.ti.com/lit/ds/symlink/ads1115.pdf).
I²C a 100 kHz con timeouts de 20 ms y espera de conversión hasta 40 ms, más
una operación en curso. Lecturas inválidas producen NaN y estado de error;
no se publican alturas viejas como mediciones válidas.

`I = V × 1000 / shunt_ohm`

`h = (I − current_min_ma) × height_span_m / (current_max_ma − current_min_ma)`

`porcentaje = 100 × (h − height_min_m) / (height_max_m − height_min_m)`

Corriente fuera de calibración ±0,4 mA produce under/over_range; es un margen
propio, sin asumir NAMUR en HY-5000. Se conserva V e I para diagnóstico, pero
altura/porcentaje quedan inválidos. Dentro del margen se limita altura al rango
y porcentaje a 0–100. IIR configurable, reiniciado ante falla/cambio de calibración.

Para validar/calibrar nivel:

1. Aplicar tensiones conocidas 0 / 0,6 / 1,8 / 3,0 V con fuente segura y multímetro.
   Raw esperados aproximados 0 / 4800 / 14400 / 24000. Objetivo inicial ≤1 % de
   referencia, considerando exactitud de instrumentos. 0 V debe indicar under_range.
2. Contrastar 4 / 12 / 20 mA reales: 0 / 2,5 / 5 m sin filtro (`level_filter_alpha=1`).
3. Medir la corriente real a mínimo/máximo y ajustar `current_min_ma` y
   `current_max_ma`. Ajustar `shunt_ohm` al valor medido del shunt desconectado.
4. Configurar las alturas útiles del depósito para el porcentaje; no cambiar
   el rango nominal del HY-5000 para representar una cisterna más baja.
5. Volver a activar el filtro y probar desconexión/recuperación del ADC y del lazo.

**Caudal inicialmente deshabilitado (`flow_enabled=0`)**: GPIO4 no se configura
hasta habilitarlo después de verificar eléctricamente el divisor y tener R2
conectada. Activar `flow_enabled=1` desde Zigbee2MQTT. El driver usa PCNT, flancos
ascendentes, filtro de 1 µs y extensión a 64 bits en cada límite de 30000 pulsos;
no borra el contador durante adquisición. No usa el acumulador interno `int`
del driver para evitar su desbordamiento a largo plazo. Snapshot estable y
monotónico; una carrera de overflow se reintenta/delega a la próxima ventana.
Sin pullups/pulldowns internos que alteren el divisor externo.

`F = pulsos / dt_real`

`Q = max(0, (F − flow_intercept) / flow_slope)`

Defaults: slope=8,1, intercept=−3. F=0 produce Q=0, independientemente del
intercepto. Valores por debajo de `flow_threshold` producen cero. Estado
`out_of_range` para Q positivo fuera de 1–30 L/min; la lectura y acumulación
usan la calibración extrapolada, cuya exactitud fuera del rango no está garantizada.
`no_flow` significa ausencia de pulsos/caudal detectable: no distingue tubería
sin flujo de sensor desconectado. Un GPIO flotante no permite medición válida.

El volumen se integra con la tasa sin filtro de cada ventana: `ΔL = Q × dt / 60`;
el filtro IIR es sólo para el caudal mostrado y se pone a cero al cesar pulsos.
No se supone un número fijo de pulsos/litro con intercepto distinto de cero.
La ventana introduce cuantización en arranque/parada y flujos bajos, mayor cuanto
más corto el intervalo; validar contra un recipiente patrón antes de usar totales.

Para calibrar caudal:

1. Con sensor aislado del GPIO, verificar niveles, transitorios y flancos del divisor.
2. Usar generador de pulsos seguro a 3,3 V: 5,1 / 78 / 240 Hz deben producir
   aproximadamente 1 / 10 / 30 L/min con defaults. Contar más de 30000 pulsos
   para verificar continuidad a través de un overflow (p.ej. 240 Hz durante 130 s).
3. Recoger un volumen conocido y cronometrar a varios caudales en rango nominal;
   calcular Q y F medios, ajustar pendiente/intercepto por regresión F=aQ+b.
4. Probar cero, habilitar/deshabilitar, ruido, pulsos estrechos y cambios de caudal.
   Confirmar ΔL por integración frente al patrón. La relación suministrada no
   ha sido validada físicamente con el YF-B6 concreto.

## Persistencia y diagnóstico

Un registro NVS versionado contiene configuración completa y total en `double`.
NVS aporta integridad del blob; se valida versión, tamaño, valores finitos y
relaciones de calibración. No se borra NVS al detectar errores. Un registro
inválido impide sobrescribir automáticamente el total con defaults; se registran
fallos y los sensores continúan. Restauración de red corresponde al SDK Zigbee.

Checkpoint de volumen cambiado cada **10 min**; configuración tras 30 s sin
cambios, con al menos 30 s entre intentos. No hay escritura por muestra.
Con almacenamiento sano, un corte puede perder hasta aproximadamente 10 min de
consumo (≈300 L a 30 L/min), más un pequeño retraso de tarea. Fallos de flash
pueden aumentar esa pérdida. La configuración puede perder cambios de los últimos
30 s. El total no se reinicia al encender ni al reflashear conservando NVS.

`reset_volume=RESET` solicita reset voluntario; se guarda cero antes de modificar
el total RAM. Si NVS falla, el total se conserva y se registra rechazo. La
respuesta ZCL acepta la solicitud: confirmar su ejecución por el reporte de
volumen y el log. Las credenciales de red no se borran al resetear volumen.

Se mide y persiste el total en doble precisión; Analog Input transmite float32,
por lo que la resolución del total disminuye para valores muy grandes. No es
el volumen de agua almacenado: estimarlo requiere geometría futura separada.

## Arquitectura y pruebas

[ARCHITECTURE.md](docs/ARCHITECTURE.md) documenta tareas, concurrencia, IDs,
tipos, escalas, unidades, estados y parámetros. [PLAN.md](docs/PLAN.md) mantiene
las fases y verificaciones. [VALIDATION.md](docs/VALIDATION.md) distingue pruebas
realizadas de las pendientes.

Pruebas de cálculo C ejecutables en la placa habilitando `TANK_SELF_TEST` en
menuconfig (deshabilitadas en el firmware final). Validan mínimo/máximo, calibración,
fallas, caudal cero/intercepto, integración y configuración. El repositorio también
incluye los headers mínimos para ejecutarlas con un compilador C de host:

```sh
cc -std=c11 -Wall -Wextra -Werror -Itests/host/include \
  -Icomponents/telemetry/include -Icomponents/level_sensor/include \
  -Icomponents/flow_sensor/include tests/host/test_measurements.c \
  components/telemetry/config.c components/level_sensor/level_sensor.c \
  components/flow_sensor/flow_math.c -lm -o /tmp/tanque-tests
/tmp/tanque-tests
```

Ver instalación y ejecución del test de conversor en `zigbee2mqtt/README.md`.
Para aceptación prolongada: mantener 24–72 h con flujo conocido por etapas,
examinar MQTT 10–15 min para duplicados, provocar reinicios/cortes antes y después
de checkpoints, contrastar total restaurado y confirmar reconexión sin reemparejar.
Probar ADC ausente sin perder Zigbee/caudal y PCNT deshabilitado sin perder nivel.
No se ejecutó todavía esta prueba prolongada ni se controlan electroválvulas.

## Repositorio

Se versionan fuentes, CMake, configuración por defecto, manifiesto y lock de
dependencias, conversor, pruebas y documentación. `sdkconfig`, builds y
`managed_components` se generan localmente y están excluidos de Git. Al clonar,
el Component Manager resuelve las versiones fijadas desde `dependencies.lock`.

No se incluyen credenciales de Home Assistant/MQTT, volcados de NVS ni binarios
de dispositivos. Los logs mencionados en VALIDATION.md son registros locales en
`/tmp` y pueden desaparecer; sus resultados relevantes están resumidos allí.
