idf_component_register(SRCS "storage.c" INCLUDE_DIRS "include"
                       REQUIRES telemetry PRIV_REQUIRES nvs_flash)
