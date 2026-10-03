# Persistencia

NVS hydraulic/state: blob versionado con calibración/configuración y volumen
acumulado double. El propietario es app_main. Checkpoint 10 min de volumen;
debounce 30 s de configuración. Fallos no borran flash. Reset voluntario
commitea cero antes de cambiar RAM. Ver README.md para pérdida tras cortes
 y docs/VALIDATION.md para pruebas pendientes de persistencia y reinicios.
