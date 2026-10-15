# Conversor externo e instalación

Archivo: `telemetria_tanque.mjs` (ESM). Reconoce fabricante MILANGAS y modelo
ESP32C6_HYDRAULIC_1. No requiere parches a zigbee-herdsman.

1. Zigbee2MQTT → Settings → Dev console → External converters: crear archivo
   `telemetria_tanque.mjs` con el contenido del proyecto. Alternativa: copiar a
   `external_converters/` junto a `configuration.yaml` de Zigbee2MQTT.
2. Si la versión requiere habilitar JS externo, activar `enable_external_js`
   según su configuración (Zigbee2MQTT ≥2.11 lo deshabilita por defecto en
   instalaciones nuevas). Verificar nombre/ruta del ajuste en la versión local.
3. Confirmar carga en el log/lista de conversores. Si se instaló por archivo,
   reiniciar Zigbee2MQTT. Abrir Permit join y alimentar ESP32.
4. Esperar interview y configure. Para dispositivo incorporado previamente,
   ejecutar Reconfigure y comprobar bindings/reporting de endpoints 10–17.
5. Verificar las nueve entidades en Zigbee2MQTT y Home Assistant vía MQTT discovery.

[Procedimiento oficial](https://www.zigbee2mqtt.io/advanced/more/external_converters.html).
Se usan `deviceAddCustomCluster`, `device.addCustomCluster`, `Endpoint.bind`,
`configureReporting`, `read`, `write` y exposes oficiales. El registro privado
se realiza al inicio de configure y en evento start mediante modernExtend.
El test usa `prepareDefinition` real para verificar orden y APIs, no sólo sintaxis.

Ejemplos MQTT hacia `zigbee2mqtt/NOMBRE/set`:

```json
{"current_min_ma":4.02,"current_max_ma":19.96,"level_filter_alpha":0.2}
```

```json
{"flow_enabled":1}
```

```json
{"reset_volume":"RESET"}
```

Activar flow_enabled sólo después de comprobar divisor y R2. Reset solicita
reinicio persistente, no debe mandarse como comando retenido. Confirmar total
reiniciado por reporte posterior; si NVS falla se conserva el total y se registra
rechazo. `no_flow` no diagnostica si el caudalímetro está físicamente conectado.

Lectura hacia `zigbee2mqtt/NOMBRE/get`:

```json
{"level_m":"","flow_state":"","volume_l":""}
```

## Validación reproducible

Node >=20.15, paquetes en /tmp sin tocar el sistema Zigbee2MQTT:

```sh
npm install --prefix /tmp/tanque-converter-validation zigbee-herdsman-converters@26.108.1 --no-audit --no-fund
cp zigbee2mqtt/telemetria_tanque.mjs tests/test_converter.mjs /tmp/tanque-converter-validation/
node --check zigbee2mqtt/telemetria_tanque.mjs
node /tmp/tanque-converter-validation/test_converter.mjs
```

Prueba actual verificada con 26.115.1: carga, prepareDefinition, registro de cluster,
9 configuraciones reporting (8 en firmware anterior), 24 exposes, unidades/mapping, NaN, rechazo de
calibración inválida y reset. El mock de endpoints no prueba radio ni respuesta
ZCL: la prueba con Zigbee2MQTT real queda pendiente. Si la instalación usa otra
versión, validar allí antes de atribuir un fallo al firmware.

## Política desde firmware 0.2.1

Muestreo y volumen local independientes de reporting: mínimo 30 s por atributo,
heartbeat 600 s y umbrales de cambio. Reemplazar el conversor y ejecutar
**Reconfigure** si el dispositivo ya estaba incorporado; no requiere reemparejar.
`publication_s` acepta 30–600 s (default 30). El test comprueba los ocho mínimos
de 30 s y máximos de 600 s, y rechaza publication_s=29.

## Volumen almacenado — firmware 0.3.0

`stored_volume_l` mide el contenido del cilindro vertical de diámetro interior
1 m y altura 1 m. `volume_l` sigue siendo el total del caudalímetro. Discovery
usa `measurement` para agua almacenada y `total_increasing` para el total.

El converter admite firmware anterior: si el dispositivo no tiene endpoint 18,
calcula litros con los reportes de `level_m`, limita la altura a 0–1 m y convierte
NaN a null. Reconfigure conserva los ocho reportes existentes. No agrega tráfico
Zigbee; un GET de `stored_volume_l` lee endpoint 10 en ese caso.
Un estado de nivel con falla invalida los litros; el fallback no usa una altura
anterior devuelta por GET mientras el último estado indica falla.

Aplicar y verificar por lectura (los defaults nuevos no reemplazan NVS):

```json
{"height_min_m":0,"height_max_m":0.7,"height_span_m":5}
```

Tras flashear **sin erase-flash**, ejecutar interview mediante
`bridge/request/device/interview` con `{"id":"Esp32-telemetria-tanque"}` para
descubrir endpoint 18 y luego Reconfigure. No hace falta borrar el nodo ni NVS.
Ahora `stored_volume_l` proviene del firmware y tiene reporting mínimo 30 s,
máximo 600 s, cambio 1 L. Verificar endpoint 18 en `bridge/devices` y comparar
35 cm → 274,89 L / 50 % y 70 cm → 549,78 L / 100 %.
Referencia: [API oficial de interview/configure](https://www.zigbee2mqtt.io/guide/usage/mqtt_topics_and_messages.html).
