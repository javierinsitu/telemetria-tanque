# Plan por fases y estado

Proyecto con ADC, hidráulica, contador, radio, conversor y NVS. Este documento
conserva el estado de implementación y los criterios de aceptación por fase.
Cada fase tiene código y criterios de prueba; no se confunde implementación con
aceptación física. No se afirma haber cumplido ensayos que requieren instrumentos.

| Fase | Resultado implementado | Verificación realizada | Pendiente para aceptación |
|---|---|---|---|
| 1 | I²C ADS1115 y diagnóstico | Compilación C6, lectura por USB, recuperación tras timeout inicial | Tensiones patrón, cableado y desconexión |
| 2 | Corriente, nivel, porcentaje, IIR, fallas | Pruebas C en placa: extremos, calibración, porcentaje, invalidación; lecturas reales | Contraste de altura con patrón |
| 3 | PCNT y overflow uint64, frecuencia, caudal, integración | Compilación; pruebas C de fórmula, cero, intercepto e integración | Divisor, periférico habilitado, generador/overflow y calibración YF-B6 |
| 4 | Router nativo, identidad, cache/reporting, commissioning | Estado SDK restaurado; coordinador real recibe mediciones, Reconfigure OK y ocho reporting 30–600 s | Ensayo de tráfico 10–15 min, heartbeat y reconexión ante corte del coordinador |
| 5 | Conversor y parámetros/entidades HA | Instalado en Z2M 2.14.2/ZHC 26.115.1; lecturas de configuración, ocho entidades HA registradas y discovery con clases estadísticas | Aceptación visual, fallas físicas y escrituras remotas/persistencia |
| 6 | Blob NVS, checkpoints, reset y diagnóstico | Inicialización sin error en placa, integridad/validación implementadas | Volumen no cero, reset remoto, fallos, cortes y 24–72 h |

Arquitectura/protocolo en ARCHITECTURE.md; registro de pruebas en VALIDATION.md;
backlog priorizado con criterios de cierre en [TODO.md](../TODO.md).
El firmware queda útil como router que busca red y mide ADC/nivel; PCNT se activa
tras verificar hardware con flow_enabled=1. Los siguientes pasos son ensayos de
sensores, tráfico prolongado y persistencia. No hace falta reemparejar para
actualizar reporting.
