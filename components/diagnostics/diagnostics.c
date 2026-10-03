#include "diagnostics.h"
#include <inttypes.h>
#include "esp_log.h"
#include "esp_timer.h"

void diagnostics_adc_result(esp_err_t result)
{
    static esp_err_t previous = ESP_OK;
    static uint64_t errors;
    static int64_t last_log_us;
    const int64_t now = esp_timer_get_time();
    if (result != ESP_OK) {
        ++errors;
        if (result != previous || now - last_log_us >= 60000000) {
            ESP_LOGW("diagnostics", "ADC: %s, errores acumulados=%" PRIu64,
                     esp_err_to_name(result), errors);
            last_log_us = now;
        }
    } else if (previous != ESP_OK) {
        ESP_LOGI("diagnostics", "Comunicacion y adquisicion ADC recuperadas");
    }
    previous = result;
}
