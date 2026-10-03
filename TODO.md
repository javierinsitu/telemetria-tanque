# TODOs y aceptación pendiente

Estado de referencia: firmware **0.2.1**, actualizado el **2026-10-03**.
El código está integrado y compilado. Las tareas siguientes distinguen ensayos
pendientes de ampliaciones futuras; no presentan funciones existentes como faltantes.
Los resultados comprobados están en [VALIDATION.md](docs/VALIDATION.md).

## Prioridad 1 — validar hardware y mediciones

- [ ] Verificar alimentación, masa común, pullups I²C, dirección ADS1115 y valor
  real del shunt. Registrar esquema efectivo y valores medidos.
- [ ] Contrastar A0 con multímetro a 0 / 0,6 / 1,8 / 3,0 V: guardar raw,
  tensión indicada y error. Objetivo inicial ≤1 % considerando instrumentos.
- [ ] Confirmar que las lecturas variables provienen del HY-5000 conectado y
  de cambios reales; no aceptar lecturas de una entrada flotante como nivel.
- [ ] Calibrar HY-5000 con alturas conocidas; ajustar corriente mínima/máxima,
  shunt y alturas útiles para porcentaje. Diferenciar rango del sensor y depósito.
- [ ] Desconectar/reconectar ADC y lazo: comprobar estados inválidos, recuperación
  del filtro y continuidad del resto del dispositivo.
- [ ] Medir el divisor YF-B6 con GPIO4 desconectado: niveles alto/bajo,
  tolerancias, transitorios y flancos. Confirmar R2 presente antes de habilitar PCNT.
- [ ] Habilitar `flow_enabled=1` y verificar inicialización PCNT y ausencia de
  pulsos espurios con flujo detenido.
- [ ] Probar generador seguro a 5,1 / 78 / 240 Hz y más de 30000 pulsos;
  comprobar continuidad de contador/volumen al cruzar overflow, sin pérdidas
  durante snapshots ni carreras con la ISR.
- [ ] Calibrar YF-B6 con recipiente patrón y cronómetro a varios caudales;
  verificar la relación suministrada F=8,1Q−3 y registrar ajuste pendiente/intercepto.
- [ ] Medir error de volumen en arranque/parada y cerca del caudal mínimo;
  elegir intervalo/umbral según cuantización real y comprobar extrapolación
  fuera de 1–30 L/min antes de utilizar esos totales.

## Prioridad 1 — validar Zigbee2MQTT y Home Assistant

- [ ] Confirmar la versión instalada de Zigbee2MQTT y ZHC y cargar el conversor
  actualizado. Validar sintaxis/carga allí, además del test con ZHC 26.108.1.
- [ ] Confirmar interview completo e identidad MILANGAS / ESP32C6_HYDRAULIC_1.
  `unido=1` observado en firmware confirma estado SDK, no recepción MQTT.
- [ ] Ejecutar Reconfigure con el conversor de 0.2.1 y verificar ocho bindings,
  Configure Reporting y Read Attributes, incluidos tipos y unidades estándar.
- [ ] Comprobar seis entidades numéricas y dos estados en Home Assistant, con
  unidades, tratamiento de NaN/null y semántica del acumulado.
- [ ] Observar 10–15 min de tráfico: mínimo 30 s por atributo, umbrales y
  heartbeat de 600 s, sin reportes manuales duplicados ni publicaciones idénticas
  repetidas. Registrar atributos y tramas por separado.
- [ ] Probar escrituras remotas válidas/inválidas de calibración y configuración,
  respuestas de lectura y persistencia; confirmar que `publication_s=29` se rechaza.
- [ ] Confirmar reconexión con recepción de datos después de reinicio y corte
  del coordinador, conservando credenciales y sin reemparejar.

## Prioridad 2 — persistencia y funcionamiento prolongado

- [ ] Acumular un volumen distinto de cero, esperar checkpoint y reiniciar:
  comprobar restauración de total y calibraciones.
- [ ] Cortar alimentación antes/después de checkpoints y comparar pérdida real
  con el intervalo de 10 min; contemplar fallos de almacenamiento y demora de tarea.
- [ ] Probar `reset_volume=RESET`: confirmar cero persistido sin afectar red ni
  calibración; si falla NVS, conservar total previo y registrar rechazo.
- [ ] Probar migración de registro v0.2.0 con intervalo de 5–29 s: elevarlo a
  30 s, preservando exactamente volumen y demás parámetros.
- [ ] Probar rechazo de registros NVS de tamaño/versión/valores inválidos y
  fallos de commit, sin borrado automático ni sobrescritura silenciosa del total.
- [ ] Investigar el timeout inicial ADS1115 observado durante el arranque de
  radio. Identificar causa con trazas/timing y comprobar una corrección; no se
  atribuye todavía a una causa demostrada.
- [ ] Ejecutar 24–72 h con sensores y coordinador reales: registrar heap,
  errores/reintentos, volumen patrón, reconexiones y frecuencia de escrituras.
- [ ] Probar fallas independientes: ADC ausente con caudal/radio activos y PCNT
  deshabilitado/fallido con nivel/radio activos.

## Prioridad 3 — mejoras y ampliaciones

- [ ] Automatizar build y pruebas de cálculo/conversor en CI con versiones fijadas.
- [ ] Añadir pruebas automatizadas de migración y persistencia, incluyendo fallos,
  sin utilizar la NVS del dispositivo de producción.
- [ ] Evaluar confirmación remota explícita del reset persistente: la respuesta
  ZCL actual acepta una solicitud; el reporte posterior confirma su ejecución.
- [ ] Evaluar un procedimiento explícito de reset de red/cambio de coordinador,
  separado del reset de volumen y sin borrado general de NVS.
- [ ] Evaluar si la pérdida potencial de 10 min es aceptable; de requerirse menor
  pérdida, diseñar checkpoints por tiempo/volumen o almacenamiento adicional
  midiendo el impacto en desgaste. No guardar por muestra automáticamente.
- [ ] Definir geometría o tabla altura–volumen configurable para estimar litros
  almacenados. Mantenerlos separados del volumen de agua que atravesó el caudalímetro.
- [ ] Evaluar resolución del total float32 para volúmenes grandes; conservar
  precisión doble en NVS y documentar una evolución compatible del protocolo.
- [ ] Diseñar sensores adicionales, electroválvulas e interlocks de riego como
  ampliaciones independientes; este firmware todavía no controla actuadores.
- [ ] Preparar releases con binarios, hashes e instrucciones reproducibles cuando
  las pruebas físicas y de integración estén aceptadas.

## Cómo cerrar un TODO

Marcar la casilla sólo con evidencia: versión de firmware/conversor, montaje,
procedimiento, resultado medido y log/resumen. Incorporar el resultado a
VALIDATION.md y actualizar PLAN.md si cambia la aceptación de una fase.
