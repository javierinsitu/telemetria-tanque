idf_component_register(SRCS "main.c" INCLUDE_DIRS "."
                       PRIV_REQUIRES adc_sensor level_sensor flow_sensor zigbee storage
                                     telemetry diagnostics esp_timer)
if(CONFIG_TANK_SELF_TEST)
    target_sources(${COMPONENT_LIB} PRIVATE "../tests/host/test_measurements.c")
endif()
