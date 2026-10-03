#include "tank_zigbee.h"
#include <math.h>
#include <stdatomic.h>
#include <string.h>
#include "esp_check.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_zigbee.h"
#include "ezbee/zha.h"
#include "ezbee/zcl/cluster/custom.h"
#include "ezbee/zcl/cluster/analog_input.h"
#include "ezbee/zcl/cluster/multistate_input.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define PRIVATE_CLUSTER 0xfc00
static const char *TAG = "zigbee";
static atomic_bool ready, joined, commissioning;
static esp_timer_handle_t retry_timer;
static uint32_t retry_ms = 5000;
static QueueHandle_t samples;
static StaticQueue_t queue_buffer;
static uint8_t queue_storage[sizeof(tank_sample_t)];
static const uint8_t manufacturer[] = {8,'M','I','L','A','N','G','A','S'};
static const uint8_t model[] = {19,'E','S','P','3','2','C','6','_','H','Y','D','R','A','U','L','I','C','_','1'};
static const uint8_t build[] = {5,'0','.','2','.','1'};
static const uint16_t units[6] = {31,98,88,82,2,5};
static const uint8_t descriptions[6][32] = {
    {7,'N','i','v','e','l',' ','m'}, {7,'N','i','v','e','l',' ','%'},
    {12,'C','a','u','d','a','l',' ','L','/','m','i','n'},
    {9,'V','o','l','u','m','e','n',' ','L'},
    {12,'C','o','r','r','i','e','n','t','e',' ','m','A'},
    {9,'V','o','l','t','a','j','e',' ','V'},
};

static esp_err_t from_ezb(ezb_err_t e) { return esp_zigbee_err_to_esp(e); }

static ezb_zcl_status_t config_check(uint16_t id, uint8_t ep, void *value)
{
    if (ep != 1 || !value) return EZB_ZCL_STATUS_INVALID_VALUE;
    if (id == 0x100) return *(uint8_t *)value <= 1 ? EZB_ZCL_STATUS_SUCCESS : EZB_ZCL_STATUS_INVALID_VALUE;
    if (id >= CFG_COUNT) return EZB_ZCL_STATUS_UNSUP_ATTRIB;
    tank_config_t c; tank_config_get(&c, NULL);
    memcpy(&c.value[id], value, sizeof(float));
    return tank_config_valid(&c) ? EZB_ZCL_STATUS_SUCCESS : EZB_ZCL_STATUS_INVALID_VALUE;
}

static void config_written(uint8_t ep, uint16_t id, void *value, uint16_t manuf)
{
    (void)ep; (void)manuf;
    if (id == 0x100) {
        if (*(uint8_t *)value) tank_request_reset();
    } else if (id < CFG_COUNT) {
        float number; memcpy(&number, value, sizeof(number));
        if (tank_config_set(id, number)) ESP_LOGI(TAG, "Configuracion id=%u valor=%g", id, number);
    }
}

static esp_err_t add_cluster(ezb_af_ep_desc_t ep, ezb_zcl_cluster_desc_t cluster)
{
    if (cluster == EZB_INVALID_ZCL_CLUSTER_DESC) return ESP_ERR_NO_MEM;
    return from_ezb(ezb_af_endpoint_add_cluster_desc(ep, cluster));
}

static esp_err_t attr_access(ezb_zcl_cluster_desc_t cluster, uint16_t attr, uint8_t access)
{
    ezb_zcl_attr_desc_t a = ezb_zcl_cluster_get_attr_desc(cluster, attr, EZB_ZCL_STD_MANUF_CODE);
    if (a == EZB_INVALID_ZCL_ATTR_DESC) return ESP_ERR_NOT_FOUND;
    return from_ezb(ezb_zcl_attr_desc_set_access(a, access));
}

static esp_err_t register_model(void)
{
    ezb_af_device_desc_t dev = ezb_af_create_device_desc();
    if (dev == EZB_INVALID_AF_DEVICE_DESC) return ESP_ERR_NO_MEM;
    for (unsigned endpoint = 0; endpoint < 9; ++endpoint) {
        const ezb_af_ep_config_t cfg = {.ep_id = endpoint ? endpoint + 9 : 1,
            .app_profile_id = EZB_AF_HA_PROFILE_ID, .app_device_id = EZB_ZHA_SIMPLE_SENSOR_DEVICE_ID,
            .app_device_version = 1};
        ezb_af_ep_desc_t ep = ezb_af_create_endpoint_desc(&cfg);
        if (ep == EZB_INVALID_AF_EP_DESC) return ESP_ERR_NO_MEM;
        if (endpoint == 0) {
            const ezb_zcl_basic_cluster_server_config_t basic_cfg = {
                .zcl_version = EZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
                .power_source = EZB_ZCL_BASIC_POWER_SOURCE_DC_SOURCE};
            ezb_zcl_cluster_desc_t basic = ezb_zcl_basic_create_cluster_desc(&basic_cfg, EZB_ZCL_CLUSTER_SERVER);
            if (basic == EZB_INVALID_ZCL_CLUSTER_DESC) return ESP_ERR_NO_MEM;
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_basic_cluster_desc_add_attr(basic,
                EZB_ZCL_ATTR_BASIC_MANUFACTURER_NAME_ID, manufacturer)), TAG, "manufacturer");
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_basic_cluster_desc_add_attr(basic,
                EZB_ZCL_ATTR_BASIC_MODEL_IDENTIFIER_ID, model)), TAG, "model");
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_basic_cluster_desc_add_attr(basic,
                EZB_ZCL_ATTR_BASIC_SW_BUILD_ID_ID, build)), TAG, "build");
            ESP_RETURN_ON_ERROR(add_cluster(ep, basic), TAG, "basic");
            const ezb_zcl_identify_cluster_server_config_t identify_cfg = {.identify_time = 0};
            ESP_RETURN_ON_ERROR(add_cluster(ep, ezb_zcl_identify_create_cluster_desc(&identify_cfg,
                EZB_ZCL_CLUSTER_SERVER)), TAG, "identify");
            const ezb_zcl_custom_cluster_config_t custom_cfg = {.cluster_id = PRIVATE_CLUSTER};
            ezb_zcl_cluster_desc_t custom = ezb_zcl_custom_create_cluster_desc(&custom_cfg, EZB_ZCL_CLUSTER_SERVER);
            if (custom == EZB_INVALID_ZCL_CLUSTER_DESC) return ESP_ERR_NO_MEM;
            tank_config_t c; tank_config_get(&c, NULL);
            for (unsigned i = 0; i < CFG_COUNT; ++i) {
                ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_custom_cluster_desc_add_attr(custom, i,
                    EZB_ZCL_ATTR_TYPE_SINGLE, EZB_ZCL_ATTR_ACCESS_READ_WRITE, &c.value[i])), TAG, "config attr");
            }
            uint8_t reset = 0;
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_custom_cluster_desc_add_attr(custom, 0x100,
                EZB_ZCL_ATTR_TYPE_BOOL, EZB_ZCL_ATTR_ACCESS_READ_WRITE, &reset)), TAG, "reset attr");
            uint16_t revision = 1;
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_custom_cluster_desc_add_attr(custom, 0xfffd,
                EZB_ZCL_ATTR_TYPE_UINT16, EZB_ZCL_ATTR_ACCESS_READ, &revision)), TAG, "revision");
            ESP_RETURN_ON_ERROR(add_cluster(ep, custom), TAG, "custom");
        } else if (endpoint <= 6) {
            const unsigned index = endpoint - 1;
            const ezb_zcl_analog_input_cluster_server_config_t analog_cfg = {
                .present_value = NAN, .status_flags = 2};
            ezb_zcl_cluster_desc_t analog = ezb_zcl_analog_input_create_cluster_desc(&analog_cfg, EZB_ZCL_CLUSTER_SERVER);
            if (analog == EZB_INVALID_ZCL_CLUSTER_DESC) return ESP_ERR_NO_MEM;
            /* Explicit descriptor preserves the standard BACnet unit codes.
             * SDK convenience header has an overly restrictive min=0x0100;
             * these metadata attributes are read-only and not runtime-written. */
            ezb_zcl_attr_desc_t unit_attr = ezb_zcl_create_attr_desc(0x75, EZB_ZCL_ATTR_TYPE_ENUM16,
                EZB_ZCL_ATTR_ACCESS_READ, EZB_ZCL_STD_MANUF_CODE, &units[index]);
            if (unit_attr == EZB_INVALID_ZCL_ATTR_DESC) return ESP_ERR_NO_MEM;
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_cluster_add_attr_desc(analog, unit_attr)), TAG, "units");
            ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_analog_input_cluster_desc_add_attr(analog,
                EZB_ZCL_ATTR_ANALOG_INPUT_DESCRIPTION_ID, descriptions[index])), TAG, "description");
            ESP_RETURN_ON_ERROR(attr_access(analog, 0x55, EZB_ZCL_ATTR_ACCESS_READ | EZB_ZCL_ATTR_ACCESS_REPORTING), TAG, "present access");
            ESP_RETURN_ON_ERROR(attr_access(analog, 0x6f, EZB_ZCL_ATTR_ACCESS_READ | EZB_ZCL_ATTR_ACCESS_REPORTING), TAG, "flags access");
            ESP_RETURN_ON_ERROR(attr_access(analog, 0x51, EZB_ZCL_ATTR_ACCESS_READ), TAG, "service access");
            ESP_RETURN_ON_ERROR(attr_access(analog, 0x1c, EZB_ZCL_ATTR_ACCESS_READ), TAG, "description access");
            ESP_RETURN_ON_ERROR(add_cluster(ep, analog), TAG, "analog");
        } else {
            const ezb_zcl_multistate_input_cluster_server_config_t status_cfg = {
                .number_of_states = endpoint == 7 ? 4 : 5, .present_value = endpoint == 7 ? 2 : 5};
            ezb_zcl_cluster_desc_t status = ezb_zcl_multistate_input_create_cluster_desc(&status_cfg, EZB_ZCL_CLUSTER_SERVER);
            if (status == EZB_INVALID_ZCL_CLUSTER_DESC) return ESP_ERR_NO_MEM;
            ESP_RETURN_ON_ERROR(attr_access(status, 0x55, EZB_ZCL_ATTR_ACCESS_READ | EZB_ZCL_ATTR_ACCESS_REPORTING), TAG, "status access");
            ESP_RETURN_ON_ERROR(attr_access(status, 0x51, EZB_ZCL_ATTR_ACCESS_READ), TAG, "service access");
            ESP_RETURN_ON_ERROR(add_cluster(ep, status), TAG, "status");
        }
        ESP_RETURN_ON_ERROR(from_ezb(ezb_af_device_add_endpoint_desc(dev, ep)), TAG, "endpoint");
    }
    const ezb_zcl_custom_cluster_handlers_t handlers = {.cluster_id = PRIVATE_CLUSTER,
        .cluster_role = EZB_ZCL_CLUSTER_SERVER, .check_value_cb = config_check,
        .write_attr_cb = config_written};
    ESP_RETURN_ON_ERROR(from_ezb(ezb_zcl_custom_cluster_handlers_register(&handlers)), TAG, "handlers");
    return from_ezb(ezb_af_device_desc_register(dev));
}

static void schedule_retry(void);
static void commissioning_cb(void *ctx)
{
    (void)ctx;
    if (ezb_bdb_dev_joined()) { atomic_store(&joined, true); return; }
    if (atomic_exchange(&commissioning, true)) return;
    ezb_err_t err = ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_NETWORK_STEERING);
    if (err != EZB_ERR_NONE) {
        atomic_store(&commissioning, false);
        ESP_LOGW(TAG, "Steering no iniciado: %d", (int)err);
        schedule_retry();
    }
}
static void retry_cb(void *ctx)
{
    (void)ctx;
    esp_err_t err = esp_zigbee_task_queue_post(commissioning_cb, NULL);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Cola Zigbee: %s", esp_err_to_name(err));
        esp_timer_start_once(retry_timer, 5000000);
    }
}
static void schedule_retry(void)
{
    if (esp_timer_is_active(retry_timer)) return;
    const uint32_t jitter = retry_ms / 5;
    esp_err_t err = esp_timer_start_once(retry_timer,
        (retry_ms - jitter + esp_random() % (2 * jitter + 1)) * 1000ULL);
    if (err != ESP_OK) ESP_LOGW(TAG, "Timer Zigbee: %s", esp_err_to_name(err));
    retry_ms = retry_ms < 150000 ? retry_ms * 2 : 300000;
}
static bool signal_handler(const ezb_app_signal_t *signal)
{
    ezb_app_signal_type_t type = ezb_app_signal_get_type(signal);
    const ezb_bdb_signal_simple_params_t *params = ezb_app_signal_get_params(signal);
    ESP_LOGI(TAG, "signal %s status=%u", ezb_app_signal_to_string(type), params ? params->status : 255);
    switch (type) {
    case EZB_ZDO_SIGNAL_SKIP_STARTUP:
        if (ezb_bdb_start_top_level_commissioning(EZB_BDB_MODE_INITIALIZATION) != EZB_ERR_NONE) schedule_retry();
        return true;
    case EZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case EZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (params && params->status == EZB_BDB_STATUS_SUCCESS) {
            if (!ezb_bdb_dev_joined()) commissioning_cb(NULL);
            else atomic_store(&joined, true);
        } else schedule_retry();
        return true;
    case EZB_BDB_SIGNAL_STEERING:
        atomic_store(&commissioning, false);
        if (params && params->status == EZB_BDB_STATUS_SUCCESS) {
            atomic_store(&joined, true); retry_ms = 5000;
            ESP_LOGI(TAG, "Unido PAN=0x%04x canal=%u direccion=0x%04x", ezb_get_panid(),
                     ezb_get_current_channel(), ezb_get_short_address());
        } else { atomic_store(&joined, false); schedule_retry(); }
        return true;
    case EZB_ZDO_SIGNAL_LEAVE:
        atomic_store(&joined, false); atomic_store(&commissioning, false); schedule_retry();
        return true;
    default: return false;
    }
}

static void stack_task(void *arg)
{
    (void)arg;
    const esp_zigbee_config_t cfg = {
        .device_config = {.device_type = EZB_NWK_DEVICE_TYPE_ROUTER,
            .install_code_policy = false, .zczr_config = {.max_children = 10}},
        .platform_config = {.storage_partition_name = "nvs",
            .radio_config = {.radio_mode = ESP_ZIGBEE_RADIO_MODE_NATIVE}}};
    esp_err_t err = esp_zigbee_init(&cfg);
    if (err == ESP_OK) err = from_ezb(ezb_bdb_set_primary_channel_set(0x07fff800));
    if (err == ESP_OK) err = from_ezb(ezb_bdb_set_secondary_channel_set(0));
    if (err == ESP_OK) err = register_model();
    if (err == ESP_OK) err = from_ezb(ezb_app_signal_add_handler(signal_handler));
    if (err == ESP_OK) err = esp_zigbee_start(false);
    if (err == ESP_OK) {
        atomic_store(&ready, true);
        ESP_LOGI(TAG, "Router listo: MILANGAS / ESP32C6_HYDRAULIC_1");
        err = esp_zigbee_launch_mainloop();
    }
    atomic_store(&ready, false);
    ESP_LOGE(TAG, "Stack detenido: %s; sensores siguen activos", esp_err_to_name(err));
    vTaskDelete(NULL);
}

static bool set_attr(uint8_t ep, uint16_t cluster, uint16_t attr, const void *value)
{
    return ezb_zcl_set_attr_value(ep, cluster, EZB_ZCL_CLUSTER_SERVER, attr,
        EZB_ZCL_STD_MANUF_CODE, (void *)value, false) == EZB_ZCL_STATUS_SUCCESS;
}

static void publisher_task(void *arg)
{
    (void)arg;
    tank_sample_t sample;
    bool have_sample = false;
    int64_t next_update = 0, next_stats = 0;
    uint32_t accepted = 0, errors = 0;
    for (;;) {
        tank_sample_t next;
        if (xQueueReceive(samples, &next, pdMS_TO_TICKS(1000)) == pdTRUE) {
            sample = next; have_sample = true;
        }
        if (!atomic_load(&ready) || !have_sample) continue;
        int64_t now = esp_timer_get_time();
        if (now < next_update) continue;
        tank_config_t c; tank_config_get(&c, NULL);
        if (!esp_zigbee_lock_acquire(pdMS_TO_TICKS(20))) {
            ++errors; next_update = now + 5000000; continue;
        }
        /* Readable cache is maintained even before joining. SDK standard
         * reporting is the sole RF producer after the coordinator configures it. */
        bool ok = true;
        bool stale = now - sample.timestamp_us > 3 * c.value[CFG_ACQUIRE_MS] * 1000;
        for (unsigned i = 0; i < 6; ++i) {
            float value = stale ? NAN : sample.value[i];
            uint8_t flags = isnan(value) ? 2 : 0;
            ok &= set_attr(i + 10, EZB_ZCL_CLUSTER_ID_ANALOG_INPUT, 0x55, &value);
            ok &= set_attr(i + 10, EZB_ZCL_CLUSTER_ID_ANALOG_INPUT, 0x6f, &flags);
        }
        uint16_t level_status = stale ? 2 : sample.level_state;
        uint16_t flow_status = stale ? 3 : sample.flow_state;
        ok &= set_attr(16, EZB_ZCL_CLUSTER_ID_MULTISTATE_INPUT, 0x55, &level_status);
        ok &= set_attr(17, EZB_ZCL_CLUSTER_ID_MULTISTATE_INPUT, 0x55, &flow_status);
        uint8_t reset = 0;
        ok &= set_attr(1, PRIVATE_CLUSTER, 0x100, &reset);
        esp_zigbee_lock_release();
        if (ok) ++accepted; else ++errors;
        next_update = now + (int64_t)c.value[CFG_PUBLISH_S] * 1000000;
        if (now >= next_stats) {
            ESP_LOGI(TAG, "Cache ZCL actualizaciones=%lu errores=%lu unido=%d",
                (unsigned long)accepted, (unsigned long)errors, atomic_load(&joined));
            next_stats = now + 300000000;
        }
    }
}

esp_err_t tank_zigbee_start(void)
{
    samples = xQueueCreateStatic(1, sizeof(tank_sample_t), queue_storage, &queue_buffer);
    const esp_timer_create_args_t timer_cfg = {.callback = retry_cb, .name = "zb_retry"};
    ESP_RETURN_ON_ERROR(esp_timer_create(&timer_cfg, &retry_timer), TAG, "timer");
    if (xTaskCreate(stack_task, "zigbee", 8192, NULL, 6, NULL) != pdPASS) return ESP_ERR_NO_MEM;
    if (xTaskCreate(publisher_task, "zb_publish", 4096, NULL, 4, NULL) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}
void tank_zigbee_update(const tank_sample_t *s) { if (samples) xQueueOverwrite(samples, s); }
