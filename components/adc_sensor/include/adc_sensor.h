#pragma once

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int16_t raw;
    float voltage_v;
    int64_t timestamp_us;
} adc_sample_t;

/* Single owner: acquisition task. All calls must be serialized by the caller.
 * Output is written only on success; never reuse it after an error.
 * Driver allocations occur once at init, not per sample.
 */
esp_err_t adc_sensor_init(uint8_t address);
esp_err_t adc_sensor_read(adc_sample_t *sample);
