#include "storage.h"
#include <math.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

typedef struct {
    uint32_t version;
    tank_config_t config;
    double volume;
} storage_record_t;
static nvs_handle_t handle;
static bool available;

esp_err_t storage_init(tank_config_t *c, double *volume)
{
    tank_config_defaults(c); *volume = 0;
    esp_err_t err = nvs_flash_init();
    /* Never erase NVS on error: it may hold network credentials or volume. */
    if (err != ESP_OK) return err;
    err = nvs_open("hydraulic", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    available = true;
    storage_record_t record;
    size_t size = sizeof(record);
    err = nvs_get_blob(handle, "state", &record, &size);
    if (err == ESP_ERR_NVS_NOT_FOUND) return ESP_OK;
    if (err != ESP_OK) { available = false; return err; }
    bool migrated = false;
    /* Version 0.2.0 allowed 5–29 s. Upgrade only that valid legacy range;
     * retain all calibration and the exact accumulated volume. */
    if (size == sizeof(record) && record.version == 1 &&
        isfinite(record.config.value[CFG_PUBLISH_S]) &&
        record.config.value[CFG_PUBLISH_S] >= 5 &&
        record.config.value[CFG_PUBLISH_S] < TANK_REPORT_MIN_S &&
        floorf(record.config.value[CFG_PUBLISH_S]) == record.config.value[CFG_PUBLISH_S]) {
        record.config.value[CFG_PUBLISH_S] = TANK_REPORT_MIN_S;
        migrated = true;
    }
    if (size != sizeof(record) || record.version != 1 ||
        !tank_config_valid(&record.config) || !isfinite(record.volume) || record.volume < 0) {
        /* Refuse to overwrite unknown/corrupt data with default volume later. */
        available = false;
        return ESP_ERR_INVALID_RESPONSE;
    }
    *c = record.config; *volume = record.volume;
    if (migrated) {
        esp_err_t save_err = storage_save(c, *volume);
        if (save_err != ESP_OK) {
            ESP_LOGW("storage", "Intervalo migrado en RAM; NVS: %s", esp_err_to_name(save_err));
        } else ESP_LOGI("storage", "Intervalo de publicacion migrado a 30 s");
    }
    ESP_LOGI("storage", "Restaurado volumen %.6f L", *volume);
    return ESP_OK;
}

esp_err_t storage_save(const tank_config_t *c, double volume)
{
    if (!available) return ESP_ERR_INVALID_STATE;
    if (!tank_config_valid(c) || !isfinite(volume) || volume < 0) return ESP_ERR_INVALID_ARG;
    storage_record_t record = {0};
    record.version = 1; record.config = *c; record.volume = volume;
    esp_err_t err = nvs_set_blob(handle, "state", &record, sizeof(record));
    return err == ESP_OK ? nvs_commit(handle) : err;
}
