#pragma once
#include "esp_err.h"

/* Single-owner state, accessed only by acquisition task. */
void diagnostics_adc_result(esp_err_t result);
