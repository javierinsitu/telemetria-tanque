#pragma once
#include "esp_err.h"
#include "telemetry.h"
typedef struct { float filtered; bool initialized; } flow_filter_t;
float flow_sensor_convert(float hz, const tank_config_t *config);
double flow_integrate(double liters, float rate_l_min, double seconds);
esp_err_t flow_sensor_init(void);
esp_err_t flow_sensor_read(const tank_config_t *config, float *rate, double *increment,
                           uint16_t *state);
void flow_sensor_filter_reset(void);
