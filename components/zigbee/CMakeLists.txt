idf_component_register(SRCS "zigbee.c" INCLUDE_DIRS "include"
                       REQUIRES telemetry PRIV_REQUIRES espressif__esp-zigbee-lib esp_timer esp_hw_support)
