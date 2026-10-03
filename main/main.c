#include <math.h>
#include "sdkconfig.h"
#include "adc_sensor.h"
#include "level_sensor.h"
#include "flow_sensor.h"
#include "tank_zigbee.h"
#include "storage.h"
#include "diagnostics.h"
#include "telemetry.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
#if CONFIG_TANK_SELF_TEST
    extern void tank_run_self_tests(void);
    tank_run_self_tests();
#endif
    ESP_LOGI("main", "Telemetria hidraulica v0.2.1: ADS1115 + PCNT + Zigbee Router");
    tank_config_t config;
    double volume;
    esp_err_t err = storage_init(&config, &volume);
    bool storage_ok = err == ESP_OK;
    if (!storage_ok) ESP_LOGE("main", "NVS: %s; no se borra flash", esp_err_to_name(err));
    tank_state_init(&config);
    ESP_LOGI("main", "Adquisicion=%.0f ms, cache/publicacion=%.0f s, checkpoint NVS=600 s",
             config.value[CFG_ACQUIRE_MS], config.value[CFG_PUBLISH_S]);
    err = tank_zigbee_start();
    if (err != ESP_OK) ESP_LOGE("main", "Inicio Zigbee: %s", esp_err_to_name(err));
    bool adc_ready = false, flow_ready = false;
    level_filter_t level = {0};
    uint32_t previous_revision = 0, saved_revision = 0;
    double saved_volume = volume;
    int64_t retry_adc_us = 0, retry_flow_us = 0;
    int64_t last_save_us = esp_timer_get_time(), config_changed_us = 0, last_log_us = 0;
    uint16_t previous_level_state = 0, previous_flow_state = 0;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        int64_t now = esp_timer_get_time();
        uint32_t revision;
        tank_config_get(&config, &revision);
        if (revision != previous_revision) {
            level.initialized = false; flow_sensor_filter_reset();
            config_changed_us = now; previous_revision = revision;
        }
        if (!adc_ready && now >= retry_adc_us) {
            err = adc_sensor_init(CONFIG_TANK_ADC_ADDRESS);
            adc_ready = err == ESP_OK;
            if (!adc_ready) ESP_LOGE("main", "I2C init: %s", esp_err_to_name(err));
            retry_adc_us = now + 5000000;
        }
        if (config.value[CFG_FLOW_ENABLED] == 1 && !flow_ready && now >= retry_flow_us) {
            err = flow_sensor_init();
            flow_ready = err == ESP_OK;
            if (!flow_ready) ESP_LOGE("main", "PCNT init: %s", esp_err_to_name(err));
            else ESP_LOGI("main", "PCNT GPIO4: flanco ascendente, filtro 1 us, contador 64 bits");
            retry_flow_us = now + 60000000;
        }
        tank_sample_t sample = {.level_state = 2, .flow_state = 3};
        for (unsigned i = 0; i < 6; ++i) sample.value[i] = NAN;
        adc_sample_t adc;
        err = adc_ready ? adc_sensor_read(&adc) : ESP_ERR_INVALID_STATE;
        diagnostics_adc_result(err);
        level_sensor_process(&level, &config, err == ESP_OK, err == ESP_OK ? adc.voltage_v : NAN, &sample);
        double increment = 0;
        err = flow_ready ? flow_sensor_read(&config, &sample.value[2], &increment,
                                            &sample.flow_state) : ESP_ERR_INVALID_STATE;
        if (err == ESP_OK && config.value[CFG_FLOW_ENABLED] == 1) volume += increment;
        else { sample.value[2] = NAN; sample.flow_state = 3; }
        if (config.value[CFG_FLOW_ENABLED] == 0) {
            sample.value[2] = NAN; sample.flow_state = 5;
        }
        if (tank_take_reset()) {
            /* Commit zero first: on failure keep the previous running total. */
            err = storage_ok ? storage_save(&config, 0) : ESP_ERR_INVALID_STATE;
            if (err == ESP_OK) {
                volume = saved_volume = 0; saved_revision = revision; last_save_us = now;
                ESP_LOGW("storage", "Volumen reiniciado voluntariamente y guardado");
            } else ESP_LOGE("storage", "Reset rechazado: %s", esp_err_to_name(err));
        }
        sample.value[3] = (float)volume;
        sample.timestamp_us = esp_timer_get_time();
        tank_zigbee_update(&sample);
        bool config_due = revision != saved_revision && now - config_changed_us >= 30000000;
        bool volume_due = volume != saved_volume && now - last_save_us >= 600000000;
        if (storage_ok && (config_due || volume_due) && now - last_save_us >= 30000000) {
            err = storage_save(&config, volume);
            if (err == ESP_OK) {
                saved_volume = volume; saved_revision = revision;
                ESP_LOGI("storage", "Checkpoint %.6f L", volume);
            } else ESP_LOGE("storage", "Checkpoint: %s", esp_err_to_name(err));
            last_save_us = now;
        }
        if (!last_log_us || now - last_log_us >= 10000000 ||
            sample.level_state != previous_level_state || sample.flow_state != previous_flow_state) {
            if (sample.level_state != previous_level_state && sample.level_state != 1) {
                ESP_LOGW("sensors", "Nivel no valido: estado=%u", sample.level_state);
            }
            if (sample.flow_state != previous_flow_state &&
                (sample.flow_state == 3 || sample.flow_state == 4)) {
                ESP_LOGW("sensors", "Caudal con falla/fuera de rango: estado=%u", sample.flow_state);
            }
            ESP_LOGI("sensors", "V=%.4f I=%.3f mA h=%.3f m %.1f%% Q=%.3f L/min total=%.6f L estados=%u/%u",
                sample.value[5], sample.value[4], sample.value[0], sample.value[1],
                sample.value[2], volume, sample.level_state, sample.flow_state);
            previous_level_state = sample.level_state; previous_flow_state = sample.flow_state;
            last_log_us = now;
        }
        xTaskDelayUntil(&wake, pdMS_TO_TICKS((uint32_t)config.value[CFG_ACQUIRE_MS]));
    }
}
