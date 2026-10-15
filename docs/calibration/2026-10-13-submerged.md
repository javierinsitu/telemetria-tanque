# Punto sumergido aproximado — 2026-10-13

Usuario: sensor sumergido «al ras del agua a unos 5 cm de la superficie».
Referencia exacta hasta la entrada de presión pendiente de aclaración.
Se tomaron publicaciones MQTT y cinco GET durante unos 145 s,
18:41:40–18:44:01 UTC, sin modificar configuración.

Con shunt 150 Ω, cero 4 mA, máximo 20 mA y rango 5 m:

| Hora UTC | Corriente indicada (mA) | Tensión ADC (V) |
|---|---:|---:|
| 18:42:16 | 4,308333 | 0,646250 |
| 18:42:51 | 4,306667 | 0,646000 |
| 18:43:35 | 4,305834 | 0,645875 |
| 18:44:00 | 4,305834 | 0,645875 |

Profundidad filtrada observada aproximadamente 9,5–9,7 cm, estado `ok`.
Señal estable en esta toma. A 5 cm exactos, con la escala nominal, se esperan
4,160 mA; usando el cero observado ayer (~3,961 mA), unos 4,121 mA.
La lectura actual correspondería a unos 10,8 cm al corregir sólo ese cero.

No se ajustó cero ni escala: la profundidad es aproximada y existe una
discrepancia relevante con la lectura. Usar este punto y el cero de ayer para
forzar una pendiente daría aproximadamente 6,9 mA/m, más del doble de los
3,2 mA/m nominales; falta una referencia física precisa para justificarlo.
Pendiente medir profundidad desde superficie hasta entrada de presión y tomar
otro punto más separado del cero. No se confirmó exactitud a otras alturas.

## Profundidad confirmada y punto a 40 cm

El usuario confirmó que la posición anterior era de **10 cm**, corrigiendo la
estimación inicial de 5 cm. Se acepta el punto anterior como 0,10 m para el
ajuste, con la precisión física indicada por el usuario.

El usuario aclaró que el primer «listo» corresponde a un descenso de
**30 cm adicionales** desde los 10 cm iniciales: profundidad aproximada
**40 cm**. El próximo punto corresponde a **70 cm**. Se corrige la
interpretación anterior de 30 cm absolutos. Toma de unos 145 s, 18:46:48–18:49:09 UTC, cinco GET y publicaciones.
Datos capturados en `2026-10-13-30cm.jsonl`. Corriente alrededor de 5,17 mA
(lecturas al final de las ventanas 5,17083 / 5,16917 / 5,17500 / 5,17500 /
5,17500 mA). Profundidad filtrada alrededor de 36,6–37,0 cm, estado `ok`.
Los atributos MQTT se actualizan por separado; no interpretar cada registro
como una muestra simultánea de todas las magnitudes.

La señal permanece suficientemente estable para conservar el punto. En esa toma no se modificaron parámetros; se conservó el punto para
evaluar la curva junto con el cero y el punto siguiente de 70 cm.

## Punto a 70 cm y calibración aplicada

Segundo «listo»: descenso adicional de 30 cm, profundidad aproximada 70 cm.
Toma 18:52:38–18:54:59 UTC, unos 145 s, cinco GET. Datos en
`2026-10-13-70cm.jsonl`. Corrientes de respuestas GET: 6,13500 / 6,13667 /
6,12417 / 6,12250 / 6,13833 mA; media 6,13133 mA. Profundidad filtrada con
calibración original aproximadamente 66,3–66,7 cm; estado `ok`.

Ajuste lineal por mínimos cuadrados, igual peso por punto, I = b + a × h:

| Profundidad de referencia (m) | Corriente usada (mA) |
|---|---:|
| 0,00 (aire, día anterior) | 3,96125 |
| 0,10 | 4,306667 |
| 0,40 | 5,173000 |
| 0,70 | 6,131334 |

b = 3,9729376 mA; a = 3,0670834 mA/m. Residuos equivalentes de profundidad:
−0,381 / +0,881 / −0,873 / +0,373 cm. Son residuos del ajuste sobre sus
propios puntos, no una prueba independiente de exactitud. Las profundidades
son aproximadas y el cero se tomó el día anterior.

Se aplicaron por MQTT y se confirmaron mediante lectura ZCL:

- `current_min_ma`: 4 → 3,97294 (leído 3,972939968).
- `current_max_ma`: 20 → 19,30835 (leído 19,308349609).
- `height_span_m=5`, shunt 150 Ω, filtro 0,2 y límites útiles 0–0,7 m conservados.

El valor máximo representa la extrapolación del ajuste a 5 m; no se ensayó
esa profundidad. Es una calibración práctica para el intervalo probado
0–0,70 m, sin modificar el rango nominal del sensor.

Tres verificaciones posteriores, en la misma posición de unos 70 cm:
0,705743 / 0,704820 / 0,707789 m, estado `ok`. La lectura resulta cercana a
la referencia; MQTT mantiene cache por atributo y no entrega muestras
simultáneas de todas las magnitudes. Datos en `2026-10-13-applied.jsonl`.
Lectura de parámetros confirmada; no se reinició la placa para comprobar
persistencia NVS. Pendiente contraste independiente en otra altura.
