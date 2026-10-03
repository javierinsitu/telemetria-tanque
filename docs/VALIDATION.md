# Registro de validación — 2026-10-02

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
| Instalar conversor en Zigbee2MQTT real | Pendiente |
| Unión, Read Attributes, reporting, ajustes remotos y HA | Pendiente |
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
