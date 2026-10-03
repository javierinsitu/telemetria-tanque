#pragma once
#include "esp_err.h"
#include "telemetry.h"
esp_err_t tank_zigbee_start(void);
void tank_zigbee_update(const tank_sample_t *sample);
