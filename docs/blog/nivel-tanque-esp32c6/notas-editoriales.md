# Notas de preparación

- Artículo técnico y experiencial, en primera persona, preparado como bundle
  Hugo: `index.md` y `cableado.png` en la misma carpeta.
- Estado `draft: true`; no se publicó ni desplegó.
- No se encontró el repositorio Hugo del blog en las ubicaciones locales
  consultadas. Categorías y tags quedan vacíos para reutilizar las taxonomías
  reales al incorporarlo al sitio. No se inventaron enlaces internos.
- Fuentes primarias consultadas: datasheet ADS1115 (TI) y documentación
  introductoria del ESP Zigbee SDK (Espressif), enlazadas en el artículo.
- Resultados contrastados con los registros locales de calibración.
- El HY-5000 se describe según este montaje y su configuración; falta su
  ficha exacta para presentar especificaciones como verificadas por fabricante.
- Propuesta para `/logs`: [2026-10-12] ESP32 Corrección de masa común y
  verificación del sensor de nivel HY-5000 fuera del agua.
- Propuesta para `/logs`: [2026-10-13] ESP32 Calibración práctica del nivel
  del tanque con puntos a 10, 40 y 70 cm y verificación por Zigbee2MQTT.
- Posible actualización de `/uses`: HY-5000, ADS1115 y ESP32-C6 Zero, si aún
  no figuran. No se modificaron páginas del sitio.
- Build completo del sitio pendiente: su repositorio y tema no están
  disponibles en este workspace. Hugo sí está instalado; se valida el
  borrador con un sitio temporal mínimo. Esto no valida el tema FixIt.

## Imagen

Generada con la herramienta integrada `image_gen`; no se usó CLI ni API key.
Prompt inicial:

```text
Use case: scientific-educational
Asset type: wiring illustration for a Spanish electronics blog article.
Primary request: Generate a clean accurate landscape infographic of a two-wire HY-5000 4–20 mA water depth sensor, ADS1115 module, ESP32-C6 Zero and external 12 V DC supply. Cream white background, crisp large Spanish labels, colored orthogonal wires, roomy layout, schematic blocks with small tasteful device illustrations rather than realistic pin layouts. Title "Sensor de nivel: cableado y masa común".
Circuit topology is mandatory: supply +12 V ONLY connects to HY-5000 red wire. HY-5000 black wire output connects to ONE junction named "A0 / señal", which branches to ADS1115 A0 and top of resistor "150 Ω". Bottom of that resistor goes to bottom common ground bus. +12 V never connects directly to A0 or ESP32. Supply negative, resistor bottom, ADS1115 GND, ADS1115 ADDR and ESP32 GND all explicitly connect with dots to single bus labelled "GND / masa común". Label supply minus "−12 V (retorno)" is INCORRECT: use "0 V / negativo" for supply negative.
ESP32 3V3 to ADS1115 VDD, ESP32 GPIO3 to ADS1115 SDA, GPIO2 to ADS1115 SCL. Show two discrete pullup resistors "4,7 kΩ" from SDA and SCL to 3V3. Keep 3V3 bus separate from 12 V. ADS1115 address caption "ADDR a GND · 0x48". ESP32 USB power small label "Alimentación USB". HY-5000 caption "2 hilos · 4–20 mA · 0–5 m". Lower caption "150 Ω: 4 mA → 0,6 V · 20 mA → 3,0 V".
Show entire loop from supply plus through sensor and resistor to supply negative. No sensor output direct ground shortcut. Color red for +12 V, orange for 3V3, purple for analog signal, blue/green I2C, black ground. Highlight only common ground bus with a pale yellow band, emphasizing the missing connection resolved. All connections functional block port labels so physical board pin orientation not implied. No caudal sensor, no mains AC, no neutral AC, no water surrounding electronics. Elegant precise technical diagram.
```

Se pidió una edición posterior para corregir exclusivamente la continuidad
del cable rojo desde el positivo de la fuente al rojo del sensor y conservar
el negativo unido sólo a la masa común. La tabla del artículo explicita las
conexiones y la ilustración no define ubicaciones físicas de pines.

Para evitar ambigüedades de trazado, se seleccionó una versión por bloques.
Prompt de esa versión:

```text
Create a professional landscape scientific-educational WIRING DIAGRAM for a Spanish electronics blog. Pure schematic colored rectangular blocks, NO photographs, NO realistic modules, NO pin headers. Cream white background, large clear navy type. Title "Nivel del tanque: cableado".
Place 4 blocks across upper half in order: "Fuente 12 V DC", "HY-5000 · 2 hilos", "ADS1115", "ESP32-C6 Zero". All wires attach to explicitly labelled terminals on block edges and are CONTINUOUS.
Supply block right edge has terminals "+12 V" and "0 V". Sensor block left edge terminal "Rojo / +" and right edge terminal "Negro / salida". A single continuous red wire goes supply +12 V terminal to sensor Rojo/+ terminal. A single purple wire goes sensor Negro/salida terminal to ADS1115 terminal "A0". Dot junction in middle of purple wire drops down through vertical resistor labelled "150 Ω" to common black GND bus along bottom. No direct ground on sensor signal, only via resistor.
Supply 0 V terminal goes down by black line to bottom GND bus. ADS1115 ports "GND" and "ADDR" each explicitly join ground bus with black lines. ESP32 GND goes down to SAME common bus. All ground junctions have dots; bus labelled "MASA COMÚN · negativo fuente + GND ADC + GND ESP32". 
Between ADS1115 and ESP32, exactly three parallel lines: orange VDD to 3V3, blue SDA to GPIO3, green SCL to GPIO2. Label at each endpoint. Place 2 separate resistors labelled "4,7 kΩ": one branches SDA to orange3V3 wire, other branches SCL to orange3V3 wire. Show dots at joins, no dot for mere crossings. Make room for readable ground paths separate from these connections. ESP32 caption "Alimentación USB"; ADS caption "ADDR a GND: 0x48"; sensor caption "4–20 mA · rango 0–5 m". At bottom below bus caption "Shunt 150 Ω: 4 mA → 0,600 V | 20 mA → 3,000 V".
CRITICAL topology: +12 V must be a single visible uninterrupted RED wire from source positive to sensor positive ONLY. Source negative only joins ground bus. 3V3 comes from ESP32 to ADC and pullups only. No source photos, no sensor photos, no device pin layout speculation, no disconnected stubs. Accurate wiring topology over decoration.
```

Última edición: situar el bus naranja de 3V3 por encima de las resistencias
pullup y conectar expresamente sus terminales superiores al bus; conservar
los terminales inferiores unidos respectivamente a SDA y SCL.

Validación: Hugo renderizó el front matter YAML, tablas y bloques de código
del artículo en un sitio temporal mínimo, con borradores habilitados.

## Ampliación del 15 de octubre: YF-B10

- Se conservan título y slug; se añaden subtítulo, descripción y fecha de modificación.
- Se documentan el reemplazo del YF-B6, activación del contador, respuesta al
  cierre, divisor de 30 kΩ / 10 kΩ en GPIO4 y masa común.
- Se incorporan el reemplazo del ESP32, los cinco ensayos con 12 y 30 litros,
  los ajustes provisionales y la validación fallida con 30 litros.
- Los resultados se contrastaron con los registros de `docs/calibration/`
  y el mantenimiento del 14 de octubre. No se presenta el caudalímetro como
  calibrado: faltan señal eléctrica, pulsos crudos y tiempo activo local.
- Se actualizó la comprobación de persistencia de parámetros: fue verificada
  tras reiniciar la placa de reemplazo.
- Fuentes añadidas: PCNT y datasheet del ESP32-C6 de Espressif; documentación
  de una variante YF-B10 de HSH-Flo solamente para colores de conductores,
  sin atribuir sus especificaciones al ejemplar del montaje.
- Propuesta para `/logs`: [2026-10-15] YF-B10: pruebas con 12 y 30 litros;
  ajuste provisional sin validación y diagnóstico eléctrico pendiente.
- Posible actualización de `/uses`: YF-B10 en lugar del YF-B6. No se editó
  el sitio ni se publicó el borrador.

## Imagen de cableado YF-B10

Archivo: `cableado-yf-b10.png`. Generada con la herramienta integrada
`image_gen`, conservando la imagen anterior del sensor de profundidad.
Inspección visual: R1 en serie entre señal y GPIO4; R2 desde ese nodo a masa;
negativos de fuente, sensor y ESP32 unidos. El cruce del positivo con el
retorno de la fuente no tiene punto de unión. La ilustración describe el
montaje ensayado y explicita que sus niveles de señal siguen pendientes
de verificar.

Prompt utilizado:

```text
Use case: scientific-educational
Asset type: accurate wiring schematic image for a Spanish electronics blog.
Primary request: A spacious clean landscape diagram showing the ACTUAL TESTED CONNECTION TOPOLOGY of a YF-B10 water flow sensor powered from 12 V DC and a resistor divider to an ESP32-C6 Zero. Cream background, navy readable Spanish labels, flat schematic rectangular blocks only. No realistic boards or physical pin locations.
Title exactly "YF-B10: cableado del montaje".
Three labeled boxes across upper portion: "Fuente 12 V DC" left, "YF-B10" center, "ESP32-C6 Zero" right. Sensor subtitle "3 hilos · salida de pulsos". ESP32 subtitle "Alimentación por USB".
EXACT circuit connections:
Supply box +12 V terminal -> single continuous RED wire -> sensor terminal "Rojo / VCC".
Supply box 0 V terminal -> BLACK wire down to common GND bus across entire bottom.
Sensor terminal "Negro / GND" -> separate BLACK wire down to same common ground bus.
Sensor terminal "Amarillo / señal" -> YELLOW horizontal wire -> resistor R1 labeled "R1 = 30 kΩ" in SERIES horizontally -> yellow wire -> one visible junction dot -> ESP32 terminal "GPIO4 / pulsos".
From that junction dot, a vertical branch goes DOWN through resistor R2 labeled "R2 = 10 kΩ" and its bottom connects to common GND bus with a dot.
ESP32 terminal "GND" -> BLACK wire down to common GND bus with a dot.
The yellow signal must reach GPIO4 ONLY through series R1, never bypass the resistor. R2 goes from GPIO4 junction to ground, NOT in series with sensor ground. There is no connection from +12 V directly to GPIO4, 3V3 or USB. No other components, no arrows for wires, no inferred open-collector transistor, no extra pullups, no HY-5000 or ADS1115.
Bottom ground bus label "GND / masa común".
Two clearly separated callout boxes BELOW the entire circuit:
"Divisor: V_GPIO4 = V_señal / 4"
"Si V_señal = 12 V → V_GPIO4 = 3 V"
One small note exactly "Esquema del montaje ensayado. Niveles de señal pendientes de verificar."
Use straight continuous orthogonal wires, junction dots only on actual connections, avoid all unnecessary crossings and leave generous space between boxes and resistors. Accurate circuit topology is essential. This is a document of the tested setup, not a claim of universal compatibility or successful calibration. No oscilloscope waveform implying measured signal.
```
