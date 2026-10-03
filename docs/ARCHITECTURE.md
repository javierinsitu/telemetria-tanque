# Arquitectura y contrato Zigbee v1

## Tareas y concurrencia

- `app_main`: única propietaria del ADC, PCNT, filtro, total y handle NVS. Muestreo
  100–10000 ms con tiempo real monotónico. Inicialización de un sensor fallida
  no bloquea al otro ni a la radio; reintentos espaciados.
- `zigbee`: stack nativo/router, callbacks y commissioning. Sin I²C ni escrituras
  NVS dentro de callbacks. Cambio de calibración protegido por mutex estático;
  validación antes de aceptar Write Attributes. Reset es sólo una solicitud.
- `zb_publish`: cola estática de profundidad uno, última muestra prevalece;
  lock Zigbee hasta 20 ms. Cache a intervalo configurado 30–600 s; si falla lock
  reintenta después de 5 s; si falla setter espera intervalo. Invalida muestras
  con antigüedad superior a tres períodos de adquisición.
- PCNT ISR: sólo extiende límite a uint64 bajo spinlock, sin logs, NVS ni Zigbee.
  No se usa acumulador `int` del driver. Snapshot compara base antes/después,
  rechaza retrocesos transitorios mientras se procesa overflow. El contador nunca
  se borra durante medición. Un bloqueo de interrupciones mayor que una vuelta
  completa del contador puede perder eventos; no debe ocurrir en operación normal
  (una vuelta ≈125 s a 240 Hz). Falta ensayo físico de overflow.
- Memoria dinámica sólo en inicialización de drivers, tareas y descriptores SDK;
  no se crean buffers ni colas crecientes por muestra. Logs de sensores cada
  10 s/estado y cache cada 5 min; transporte IDF puede emitir sus propios errores.

## Identidad y modelo

Fabricante `MILANGAS`, modelo `ESP32C6_HYDRAULIC_1`, build `0.2.1`.
Perfil HA `0x0104`, dispositivo Simple Sensor (`EZB_ZHA_SIMPLE_SENSOR_DEVICE_ID`),
Router nativo IEEE 802.15.4, DC permanente. Endpoint 242 puede ser agregado por SDK.
No se toma un manufacturer code registrado de otro fabricante: cluster privado
usa frame ZCL sin campo manufacturer-specific y atributos con
`EZB_ZCL_STD_MANUF_CODE=0`. Es un protocolo local definido por fingerprint;
no implica asignación CSA ni producto certificado.

| Endpoint | Cluster | PresentValue | Tipo | Unidad BACnet | Propiedad MQTT |
|---|---|---|---|---|---|
| 1 | Basic 0x0000, Identify 0x0003, Config 0xFC00 | — | — | — | identidad/configuración |
| 10 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 31 metros | level_m |
| 11 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 98 porcentaje | level_percent |
| 12 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 88 litros/minuto | flow_l_min |
| 13 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 82 litros | volume_l |
| 14 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 2 miliamperios | sensor_current_ma |
| 15 | Analog Input 0x000C | 0x0055 | SINGLE 0x39 | 5 voltios | adc_voltage_v |
| 16 | Multistate Input 0x0012 | 0x0055 | UINT16 0x21 | estados 1–4 | level_state |
| 17 | Multistate Input 0x0012 | 0x0055 | UINT16 0x21 | estados 1–5 | flow_state |

Sin escalas implícitas: float en unidad de tabla. Analog Input también expone
Description 0x001C (CHAR_STR 0x42), OutOfService 0x0051 (BOOL 0x10, false),
StatusFlags 0x006F (BITMAP8 0x18: bit1 fault cuando NaN), EngineeringUnits
0x0075 (ENUM16 0x31). PresentValue y StatusFlags son readable/reportable, se
restringe escritura de medidas. NaN es no-value IEEE; conversor produce null.

El header SDK del helper de EngineeringUnits tiene mínimo 0x0100 incompatible
con códigos estándar BACnet 0–255. Se crean estos metadatos de lectura mediante
el descriptor genérico oficial, sin reutilizar unidades arbitrarias. Comprobado
registro/arranque en placa; pendiente Read Attributes por coordinador.
Fuentes de unidades y protocolo:
[Enumeración BACnet](https://github.com/bacnet-stack/bacnet-stack/blob/master/src/bacnet/bacenum.h),
[clusters herdsman](https://github.com/Koenkk/zigbee-herdsman/blob/master/src/zspec/zcl/definition/cluster.ts).
Flow Measurement estándar tiene una escala distinta; Analog Input evita perder
resolución o declarar L/min sobre un atributo con otra unidad.

Estados nivel: 1 ok, 2 adc_error, 3 under_range, 4 over_range.
Estados caudal: 1 no_flow, 2 flowing, 3 pcnt_error, 4 out_of_range, 5 disabled.
No flujo no prueba integridad del Hall. Sólo el estado de nivel puede diagnosticar
corriente fuera de calibración; no se afirma cuál falla física específica ocurrió.

## Configuración privada 0xFC00, endpoint 1

Todos los atributos 0–13 son SINGLE 0x39, readable/writable, sin reporting;
respuestas de lectura devuelven calibración vigente. Se acepta un valor por
operación, validando toda la configuración candidata. Para cambiar límites,
ajustar primero el que permita conservar min < max durante cada escritura.

| ID | Propiedad | Default | Límites |
|---|---|---|---|
| 0 | acquisition_ms | 1000 | 100–10000, entero |
| 1 | publication_s | 30 | 30–600, entero |
| 2 | shunt_ohm | 150 | 100–200 |
| 3 | current_min_ma | 4 | 2–6 |
| 4 | current_max_ma | 20 | 15–22 y > mínimo |
| 5 | height_span_m | 5 | >0–10 |
| 6 | height_min_m | 0 | ≥0 y < máximo |
| 7 | height_max_m | 5 | ≤10 y > mínimo |
| 8 | level_filter_alpha | 0,2 | >0–1 |
| 9 | flow_slope | 8,1 | 0,1–100 Hz/(L/min) |
| 10 | flow_intercept | −3 | −100–100 Hz |
| 11 | flow_threshold | 0,5 | 0–30 L/min |
| 12 | flow_filter_alpha | 0,3 | >0–1 |
| 13 | flow_enabled | 0 | 0/1 |
| 0x0100 | resetVolume | false | BOOL 0x10; true solicita reset, firmware vuelve a false |
| 0xFFFD | clusterRevision | 1 | UINT16 0x21, sólo lectura |

Config sin finitos/denominadores válidos devuelve INVALID_VALUE. Los defaults
IIR son por muestra: modificar intervalo cambia su constante temporal efectiva.
La calibración remota no modifica el cableado ni protege contra sobretensiones.
La pendiente inicial YF-B6 proviene de la documentación aportada por el usuario,
no se presenta como verificada para todos los modelos comercializados con ese nombre.

## Reporting

Un solo productor de RF: reporting automático SDK configurado por Zigbee2MQTT.
No hay Report Attributes manual, heartbeat duplicado ni publicación en ISR.
Por defecto mínimo 30 s, máximo 600 s y delta 0,01 m / 1 % / 0,2 L/min /
1 L / 0,1 mA / 0,015 V; estados delta 1. Cache actualizada cada 30 s, también
antes de la unión. SDK decide el reporte según configuración y bindings.
Hasta 8×120=960 atributos/h si todos cambian al mínimo; con valores estables,
8×6=48 atributos/h por máximo. Son atributos, no cantidad de tramas ni prueba
 de emisión efectiva (SDK puede agrupar). El coordinador puede cambiar límites.
Reintentos de unión con jitter ±20 %, 5 s hasta 5 min, sin timers solapados.
Credenciales de red restauradas por SDK. Leave inicia búsqueda nuevamente.

## Futuras ampliaciones

Nivel y acumulado de circulación son independientes. Geometría del depósito
requiere un módulo posterior (tabla altura–volumen o forma parametrizada) para
estimar litros almacenados. Electroválvulas/riego requieren endpoints nuevos,
interlocks y tratamiento de fallas; no se activan salidas con este firmware.

Migración de configuración: intervalos enteros de 5–29 s guardados por v0.2.0
se elevan a 30 s y se persisten una vez, conservando el resto del registro. La
validación de configuraciones nuevas rechaza valores inferiores a 30 s. Reducción
respecto al mínimo previo de 10 s: hasta 67 % menos atributos/h si todos cambian;
con señales estables no cambia el heartbeat de 600 s. Actualizar conversor y
Reconfigure para cambiar las tablas de reporting del nodo unido. Sin reemparejado.
