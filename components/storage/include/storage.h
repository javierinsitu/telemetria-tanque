#pragma once
#include "esp_err.h"
#include "telemetry.h"
esp_err_t storage_init(tank_config_t *config, double *volume);
esp_err_t storage_save(const tank_config_t *config, double volume);
