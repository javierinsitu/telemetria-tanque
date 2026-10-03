# Caudal

PCNT oficial, flancos ascendentes, filtro 1 us, extensión de overflow a uint64,
snapshot monotónico y caudal con pendiente/intercepto. Integración sin filtro
para volumen; IIR sólo para visualización. Deshabilitado por defecto hasta
verificar divisor externo; flow_enabled=1 inicia el periférico. Ver README.md.
Ensayo físico de pulsos/overflow y calibración hidráulica pendientes.
