# Registro de validación — 2026-10-02

## Reemplazo de placa — 2026-10-14

Nuevo IEEE `0xacebe6fffe2c8c5c`, firmware 0.3.0 flasheado por USB-JTAG con
Verify OK de bootloader, tabla y aplicación. NVS conservada. Emparejamiento,
nueve reportes 30–600 s y los 14 parámetros restaurados verificados por
Zigbee; calibración de nivel y contador habilitado conservados tras reset.
25 entidades de Home Assistant migradas manteniendo entity_id y preferencias;
historial anterior de nivel consultado correctamente. YF-B10 identificado,
con calibración pendiente. Nivel under_range al terminar con señal ~0 V y
fuente de 12 V previamente desconectada; no se validó lectura hidráulica en
esta placa. Detalles y respaldos en
[registro de reemplazo](maintenance/2026-10-14-esp32-replacement.md).

## Flasheo 0.3.0 y endpoint 18 — 2026-10-05

- ESP32-C6 presente como USB 303a:1001. Sin ttyACM: kernel activo
  7.2.6-arch2-1, módulos instalados 7.2.8-arch1-2. Se usó USB-JTAG nativo,
  OpenOCD Espressif v0.12.0-esp32-20260424 y `board/esp32c6-builtin.cfg`.
- Bootloader 0x0 y aplicación 0x10000 programados con **Verify OK**;
  tabla 0x8000 ya coincidía (**Existing flash content matches**). Reset run
  ejecutado; no se escribió NVS ni se hizo erase-flash.
- Imagen `/tmp/telemetria-tanque-volume-build/telemetria_tanque.bin`, SHA256
  `8497c2e5c3b25e613cef5609af1b69ff5ae0900d5776c95bbf2406ca79beda13`.
  Log `/tmp/telemetria-tanque-volume-flash.log`.
- Se conservó el emparejamiento. Interview y Configure mediante API MQTT:
  ambos `status=ok`; `bridge/devices` confirma build **0.3.0**, endpoint 18
  Analog Input, binding al coordinador y nueve reportes configurados, mínimo
  30 s, máximo 600 s. Endpoint 18 cambio 1 L.
- GET confirma rango 5 m, mínimo 0, máximo 0,7 m, estado ok y
  `stored_volume_l=785.398193359375` procedente del endpoint 18; total del
  caudalímetro 0 L y caudal deshabilitado. La altura leída de 1,664 m excede
  el tanque, por lo que el volumen se limita a la capacidad física. No es
  evidencia de calibración: siguen pendientes montaje/alturas patrón.
- Verificación de versión y lectura por Zigbee confirma arranque y acceso
  al endpoint nuevo; no se capturó consola serie ni se ejecutó el ensayo
  prolongado de reportes espontáneos/heartbeat.

## Litros almacenados — 2026-10-05, firmware 0.3.0

- Geometría confirmada por el usuario: cilindro vertical, diámetro interior
  1 m, altura física 1 m, flotador a 70 cm, HY-5000 0–5 m en el fondo.
- Se agregó `stored_volume_l` en endpoint 18, preservando los IDs existentes,
  total del caudalímetro, blob NVS y credenciales. Defaults de nivel útil 0–0,7 m;
  rango nominal 5 m. Volumen limitado por altura física 1 m, no por el flotador.
- Build ESP-IDF 6.0.2 / SDK Zigbee 2.0.3: OK, aplicación 0x84da0 (544160 bytes),
  libre 48 % de partición de 1 MB. Directorio `/tmp/telemetria-tanque-volume-build`.
  **No se flasheó 0.3.0**: endpoint 18 y sus reportes en radio quedan pendientes.
- Tests C ejecutados en host con TCC temporal, sin instalar compilador en el
  sistema: nivel, 0/35/70/80/100 cm, volumen 274,89/549,78/628,32/785,40 L,
  límite físico, IIR, invalidación ADC/lazo, configuración e integración: OK.
- `node --check`, `prepareDefinition` y tests con ZHC 26.115.1: OK. Nueve
  reportes/24 exposes, ocho reportes en firmware previo, fallback geométrico,
  endpoint 18, null, fallas y GET compatible. Discovery de litros almacenados
  usa `measurement`; el total mantiene `total_increasing`.
- Converter desplegado mediante API MQTT oficial, respuesta `status=ok` y
  checksum remoto/local idéntico. Backup final en el servidor:
  `/config/zigbee2mqtt/codex-backup-tanque-20261005-095138`.
  Reconfigure del nodo actual respondió `ok`, mantiene ocho reportes 30–600 s.
- Escritura individual y lecturas posteriores confirmaron `height_min_m=0`,
  `height_max_m=0.699999988079071` (float32 de 0,7), `height_span_m=5`.
  No se reinició el total ni se habilitó caudal. No se comprobó todavía
  persistencia de esos parámetros tras cortar alimentación.
- Discovery MQTT y registro HA confirman entidad habilitada
  `sensor.esp32_telemetria_tanque_stored_volume_l`, L, clase `measurement`.
  Lectura real recibida: `level_m=0.906245768070221`,
  `stored_volume_l=711.7637618290614`, porcentaje 100, estado `ok`.
  Sólo verifica fórmula/transporte, no exactitud física del sensor.
- Durante las consultas hubo también `adc_error`, NaN y alturas superiores al
  tanque; se agregó invalidación de litros ante estado de falla, incluso si un
  GET trae una altura anterior. Pendientes montaje/calibración con alturas
  conocidas y ensayo prolongado; no se aceptan esas lecturas como calibradas.
- Tráfico estimado con 0.3.0: máximo 1080 atributos/h si cambian las nueve
  magnitudes/estados al mínimo, heartbeat estable 54/h. El fallback instalado
  conserva 960/48 del firmware previo, pues sólo calcula a partir de mensajes
  existentes. No se realizó un ensayo RF prolongado de 10–15 min.

## Firmware 0.2.0 integrado

- ESP-IDF 6.0.2, esp-zigbee-lib 2.0.3 descargado por Component Manager, target
  ESP32-C6. Se inspeccionaron headers nativos y se resolvió durante compilación
  el nombre del enum de tipo ENUM16. No se usa API de compatibilidad v1.x.
- Build `/tmp/telemetria-tanque-integrated-build`: exit 0; binario final
  `0x84ad0` = **543440 bytes**, partición `0x100000`, libre `0x7b530` (48 %).
  No warnings de compilador de componentes propios en el build final.
  Notas Kconfig NimBLE/FATFS provienen de ESP-IDF.
- Flasheado por `/dev/ttyACM0`: bootloader 0x0, particiones 0x8000 y aplicación
  0x10000; **Hash of data verified**. No erase-flash ni escritura de partición NVS.
  Flash configurada a 8 MB, coincide con chip detectado.
- Pruebas C ejecutadas en la placa con `TANK_SELF_TEST=y`: mensaje **OK** para
  extremos nivel/porcentaje, calibración, invalidación, caudal/intercepto/cero,
  integración y configuración inválida. Firmware final tiene pruebas desactivadas.
  No había `cc` de host en el entorno; se ejecutaron con toolchain y CPU reales.
- Arranque del stack: `Router listo: MILANGAS / ESP32C6_HYDRAULIC_1`, sin error
  de creación/registro del modelo. Cache ZCL `actualizaciones=1 errores=0`.
  Se observó búsqueda de red con backoff y `BDB Steering status=3`: **no se
  confirmó incorporación al coordinador**.
- Lecturas ADC/nivel siguen durante steering: en captura de 55 s aparecen
  1,0060 / 1,5688 / 1,1213 / 1,1151 / 2,0946 / 1,4971 V. No demuestran
  precisión ni cableado del HY-5000. Hubo un timeout de adquisición al inicio
  durante carga de radio, recuperado en el siguiente ciclo (~1,3 s); no se
  observaron panic/watchdog ni repetición del fallo en esa ventana.
- PCNT implementado pero **deshabilitado por defecto** (`flow_enabled=0`, estado
  disabled). No se validó GPIO4, divisor, sensor, filtro ni overflow físicamente.
- NVS inicializada sin error visible; no se demostró aún recuperación de volumen
  distinto de cero, reset remoto, pérdida por corte ni inyección de fallas.

Capturas locales:

- `/tmp/telemetria-tanque-integrated-build.log`
- `/tmp/telemetria-tanque-flash.log`
- `/tmp/telemetria-tanque-integrated-monitor.log` (con self-tests)
- `/tmp/telemetria-tanque-final-monitor.log` (55 s sin self-tests)
- `/tmp/telemetria-tanque-release-monitor.log` (versión final 0.2.0)

## Conversor

`node --check zigbee2mqtt/telemetria_tanque.mjs`: OK.
Prueba contra paquete instalado real **zigbee-herdsman-converters 26.108.1**:
prepareDefinition, orden de registro privado, ocho bindings/configuraciones,
23 exposes, mapping por endpoint, NaN/null, validación de parámetros y reset: OK.
Los endpoints del test son mocks: no demuestra respuesta ZCL ni descubrimiento HA.

Se intentó leer la instalación Home Assistant conocida por SSH en 192.168.0.150:
**Connection refused**, por lo que no se instaló remotamente el conversor ni se
verificó la versión Zigbee2MQTT de ese equipo. Instrucciones de instalación listas
 en zigbee2mqtt/README.md. Se solicitó abrir Permit join para comprobar unión.

## Criterios pendientes

| Prueba | Estado |
|---|---|
| Alimentación, shunt, pullups, ADDR y masa común | Pendiente |
| Tensiones 0/0,6/1,8/3,0 V frente a multímetro | Pendiente |
| Desconexión y recuperación física del ADS1115/lazo | Pendiente |
| Alturas patrón y calibración HY-5000 | Pendiente |
| Divisor GPIO4 y generador de pulsos/overflow | Pendiente |
| Calibración YF-B6 por recipiente y cronómetro | Pendiente |
| Instalar conversor en Zigbee2MQTT real | OK, 2026-10-03; ver actualización remota debajo |
| Unión, Read Attributes, reporting, ajustes remotos y HA | Recepción, lecturas, reporting y registro HA OK; escrituras y aceptación física pendientes |
| Reconexión de red tras reinicio | Pendiente |
| Reset voluntario persistente y total no cero tras cortes | Pendiente |
| Ensayo prolongado 24–72 h y tráfico MQTT 10–15 min | Pendiente |

La implementación está integrada; las seis fases todavía no están aceptadas
físicamente. Ver procedimiento reproducible y pérdidas NVS en README.md.

## Historial de fase 1

Compilación original 180432 bytes; firmware flasheado por el usuario y comprobado
por USB durante 35 s: 2,249500 / 2,303000 / 0,934125 / 2,159750 V, sin errores
visibles. En ese momento imagen de 4 MB sobre flash de 8 MB; corregido en v0.2.0.
La sesión antes de habilitar acceso completo no podía abrir dispositivos USB.

## Ajuste de tráfico — firmware 0.2.1

- Default y mínimo de publication_s elevados de 10 s (mínimo permitido 5 s) a
  30 s; muestra de adquisición por defecto 1000 ms. Migración conservadora de
  intervalos persistidos 5–29 s, sin modificar volumen ni calibración. La ruta
  de migración está implementada; no se forzó un registro antiguo en la placa.
- Conversor: ocho mínimos de reporting 30 s, máximos 600 s, mismos deltas;
  prepareDefinition/test ejecutados contra ZHC 26.108.1: OK. También se probó
  rechazo de publication_s=29 y aceptación de 30.
- Build exit 0 sin warnings propios; binario 543984 bytes (0x84cf0), libre
  0x7b310 (48 %) de partición de 1 MB.
- Flash por USB: Hash of data verified. Arranque capturado de v0.2.1 con
  `Adquisicion=1000 ms, cache/publicacion=30 s, checkpoint NVS=600 s`.
- Captura: /tmp/telemetria-tanque-30s-monitor.log. Confirmación de RF y reducción
  real de tráfico todavía depende de aplicar conversor/Reconfigure y observar
  Zigbee2MQTT 10–15 min; no se afirma que la configuración haya sido aplicada
  remotamente al coordinador.
- Estimación con las ocho entidades cambiando al mínimo: de 2880 a 960
  atributos/h (≈67 % menos); heartbeat estable sigue en 48 atributos/h. No
  equivale a una única trama cada 30 s ni limita comandos/consultas del coordinador.
- En esta captura, BDB Device Reboot devolvió status=0 y el SDK indicó
  `unido=1`: se restauró el estado de red guardado. La medición ADC se recuperó
  tras el timeout inicial y continuó. Esto no sustituye una confirmación APS/MQTT
  ni verifica que el coordinador ya tenga el reporting de 30 s.

## Actualización remota del conversor — 2026-10-03

- Acceso SSH autenticado a Home Assistant 192.168.0.150 con Terminal & SSH.
  Versiones reales obtenidas de `bridge/info`: Zigbee2MQTT 2.14.2,
  zigbee-herdsman-converters 26.115.1 y zigbee-herdsman 10.10.0.
- El dispositivo `Esp32-telemetria-tanque` (IEEE `0xacebe6fffe2c8fd0`)
  estaba entrevistado, con identidad MILANGAS / ESP32C6_HYDRAULIC_1,
  definición genérica y reporting analógico previo de mínimo 0/máximo 65000 s.
- Instalación por la API MQTT oficial `bridge/request/converter/save`:
  respuesta `status=ok`, definición `source=external` y checksum del archivo
  remoto idéntico al local. Ruta:
  `/config/zigbee2mqtt/external_converters/telemetria_tanque.mjs`.
- Copias previas de configuration.yaml, database.db y state.json en
  `/config/zigbee2mqtt/codex-backup-tanque-20261003-095823`.
  Copia adicional con la primera versión instalada en
  `/config/zigbee2mqtt/codex-backup-tanque-20261003-100133`.
  Los backups permanecen en el servidor; no se incorporan al repositorio.
- Reconfigure mediante `bridge/request/device/configure`: `status=ok`.
  `bridge/devices` confirma ocho bindings y ocho reporting de mínimo 30 s,
  máximo 600 s y los deltas del conversor. Lecturas de las seis magnitudes,
  los dos estados y los 14 parámetros recibidas por MQTT; `publication_s=30`,
  `level_state=ok`, `flow_enabled=0` y `flow_state=disabled`.
- La verificación real de discovery detectó que `expose.homeassistant.state_class`
  era ignorado por Z2M. Se corrigió usando el hook oficial
  `meta.overrideHaDiscoveryPayload`, se recargó y reinició sólo Zigbee2MQTT.
  Discovery MQTT comprobado: cinco magnitudes `measurement`, volumen
  `total_increasing`, unidades m / % / L/min / L / mA / V y dos estados.
- El registro `core.entity_registry` contiene las ocho entidades esperadas,
  habilitadas. No se verificó su estado actual por REST: el token del contenedor
  SSH recibe HTTP 401 en el proxy Core; recepción MQTT y discovery sí comprobados.
- `node --check` y pruebas del conversor pasan con ZHC 26.115.1, incluidos
  el hook de discovery, ocho bindings, configuración y decodificación.
  Node local 26.9.0 queda fuera del rango declarado por estos paquetes (warning
  npm); la carga y ejecución reales en Zigbee2MQTT también fueron comprobadas.
- No se flasheó ni borró NVS, ni se habilitó caudal o reinició volumen.
  Pendientes: ensayo de tráfico 10–15 min/heartbeat, escrituras remotas,
  fallas físicas y funcionamiento prolongado. Varias publicaciones MQTT por
  ciclo son esperables: reporting estándar es por atributo, no un paquete único.
