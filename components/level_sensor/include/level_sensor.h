#pragma once
#include "telemetry.h"
typedef struct { float filtered_m; bool initialized; } level_filter_t;
void level_sensor_process(level_filter_t *filter, const tank_config_t *config,
                          bool adc_ok, float voltage, tank_sample_t *sample);
