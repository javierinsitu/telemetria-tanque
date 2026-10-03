idf_component_register(SRCS "flow_sensor.c" "flow_math.c" INCLUDE_DIRS "include"
                       REQUIRES telemetry PRIV_REQUIRES esp_driver_pcnt esp_driver_gpio esp_timer)
