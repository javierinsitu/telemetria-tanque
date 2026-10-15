# Sustitución del ESP32-C6 — 2026-10-14

El usuario reemplazó la placa anterior por una posible falla física, mantuvo
el cableado y solicitó flasheo y sustitución en Home Assistant. Se conservó
la calibración práctica de profundidad; la del YF-B10 sigue pendiente.

## Identidades

| Dato | Anterior | Nuevo |
|---|---|---|
| IEEE Zigbee | 0xacebe6fffe2c8fd0 | 0xacebe6fffe2c8c5c |
| Device ID de Home Assistant | 9171b9589eaf2a8c218d70b20178de78 | fb234486dd1a8ee24644affb9a8149c2 |

Nombre operativo conservado: `Esp32-telemetria-tanque`.

## Build y programación

- Firmware 0.3.0, ESP-IDF 6.0.2 y esp-zigbee-lib 2.0.3.
- Nuevo C6 revisión v0.2, flash real de 8 MB, USB 303a:1001.
- Kernel local 7.2.8-arch1-2 con módulos 7.2.9-arch1-1: sin `cdc_acm` ni
  ttyACM. Se usó USB-JTAG nativo con OpenOCD Espressif 20260424.
- Build en `/tmp/telemetria-tanque-replacement-build`: correcto, aplicación
  544160 bytes (0x84da0), 48 % de la partición de 1 MB libre.
- SHA256 de aplicación:
  `149ac125f56a3e96a8914253fba45f403cb9bac48beda3d9c68cbb6e70df23f0`.
- Bootloader 0x0, tabla 0x8000 y aplicación 0x10000: **Verify OK** en los tres.
- NVS 0x9000, tamaño 0x6000, respaldada localmente en
  `/tmp/telemetria-tanque-new-nvs-before.bin`; no se flasheó ni borró NVS.
  Ese archivo es privado y temporal; no se incorpora al repositorio.

## Restauración y Zigbee

Se abrió incorporación durante un máximo de 254 s y se cerró al restaurar.
Interview exitoso, dispositivo soportado, rol Router y build 0.3.0.
Se restauraron los 14 parámetros de la placa anterior y se comprobaron
mediante Read Attributes. Configuración reproducible en
[`installed-config.json`](../calibration/installed-config.json).

Calibración de nivel: mínimo 3,97294 mA, máximo 19,30835 mA, rango 5 m,
shunt 150 Ω, nivel útil 0–0,70 m y filtro 0,2.
Caudal habilitado, pendiente 8,1 e intercepto −3 provisionales del YF-B6.
El reemplazo tiene acumulado inicial **0 L**; no se transfirió al firmware
el total anterior de 11,69965 L, que quedó en respaldo e historial de HA.
No se emitió el comando de reinicio de volumen.

Se reinició por JTAG después de esperar el guardado de configuración:

- Consola USB leída directamente con libusb, sin instalar módulos del host.
- Arranque 0.3.0 y restauración NVS sin error de almacenamiento.
- Inicialización PCNT GPIO4 confirmada.
- `BDB Device Reboot status=0`, cache Zigbee `unido=1`.
- Calibración y `flow_enabled=1` comprobados nuevamente por Zigbee tras reset.
- Timeout ADC inicial recuperado en el siguiente ciclo; sin panic/watchdog
  observado durante la captura de consola de 55 s.

Se repitieron Interview y Configure, ambos con respuesta `ok`. Nueve reportes,
mínimo 30 s, máximo 600 s, manteniendo endpoints y deltas.
La descripción del conversor pasó a YF-B10 con calibración pendiente;
despliegue por `converter/save`, checksum local/remoto idéntico:
`48b129d32db2c8d7deaddf5f0754403d8e8a38f9f8f8271af80e94c336fe6658`.
`node --check` correcto. No cambió la conversión numérica de caudal.

## Migración de Home Assistant

Backup remoto antes de retirar el anterior:
`/config/zigbee2mqtt/codex-backup-tank-replacement-20261014-233527`.
Incluye configuración, base/cache de Z2M, conversor, registros de entidades
y dispositivos de HA y mapeo de migración. No se copió a Git.

El dispositivo anterior se retiró por la API MQTT de Z2M (force, por estar
averiado y offline) y el nuevo recibió su nombre. Por la API WebSocket
autenticada de Home Assistant se asignaron a las 25 entidades nuevas los
entity_id anteriores, junto con sus opciones y preferencias aplicables.
Los unique_id corresponden al IEEE nuevo. El diagnóstico linkquality
conserva su entity_id anterior y permanece deshabilitado por integración.

Se verificó:

- 25 entidades nuevas, con los 25 entity_id anteriores.
- Dispositivo anterior y sus unique_id ausentes del registro activo.
- Modelo visible actualizado a YF-B10 con calibración pendiente.
- Historial anterior de nivel accesible por el mismo entity_id.
- Sin referencias al antiguo device_id en automations.yaml, scripts.yaml
  ni dashboards Lovelace de storage consultados.
- Sin reiniciar Home Assistant ni Zigbee2MQTT y sin editar sus registros
  `.storage` en vivo: los cambios se hicieron por APIs.

## Estado al terminar

Zigbee operativo y caudal `no_flow`, 0 L/min, acumulado 0 L.
ADC comunicado, tensión alrededor de 0 V y nivel `under_range`, con altura,
porcentaje y litros almacenados inválidos. Esto es compatible con la fuente
de 12 V desconectada que indicó el usuario; no se interpreta como un cero
hidráulico ni confirma sensor de profundidad alimentado.

Pendiente alimentar el lazo y contrastar lectura física, y calibrar el
YF-B10 con volumen conocido. La prueba anunciada de 12 L no se aceptó como
calibración porque no se obtuvieron lecturas durante el fallo de la placa
anterior.
