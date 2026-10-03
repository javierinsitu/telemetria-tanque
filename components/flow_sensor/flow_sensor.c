#include "flow_sensor.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define COUNT_LIMIT 30000
static pcnt_unit_handle_t unit;
static pcnt_channel_handle_t channel;
static portMUX_TYPE count_mux = portMUX_INITIALIZER_UNLOCKED;
static uint64_t overflow_base;
static uint64_t previous;
static int64_t previous_us;
static flow_filter_t filter;

static bool overflow_cb(pcnt_unit_handle_t u, const pcnt_watch_event_data_t *event, void *ctx)
{
    (void)u; (void)ctx;
    if (event->watch_point_value == COUNT_LIMIT) {
        portENTER_CRITICAL_ISR(&count_mux);
        overflow_base += COUNT_LIMIT;
        portEXIT_CRITICAL_ISR(&count_mux);
    }
    return false;
}

esp_err_t flow_sensor_init(void)
{
    if (unit) return ESP_ERR_INVALID_STATE;
    const pcnt_unit_config_t cfg = {.low_limit = -1, .high_limit = COUNT_LIMIT};
    esp_err_t err = pcnt_new_unit(&cfg, &unit);
    if (err != ESP_OK) return err;
    const pcnt_chan_config_t ch = {.edge_gpio_num = 4, .level_gpio_num = -1};
    err = pcnt_new_channel(unit, &ch, &channel);
    if (err != ESP_OK) goto cleanup;
    /* External R2 holds the line low; internal pulls would load the divider. */
    err = gpio_set_pull_mode(GPIO_NUM_4, GPIO_FLOATING);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_channel_set_edge_action(channel, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                       PCNT_CHANNEL_EDGE_ACTION_HOLD);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_channel_set_level_action(channel, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                        PCNT_CHANNEL_LEVEL_ACTION_KEEP);
    if (err != ESP_OK) goto cleanup;
    const pcnt_glitch_filter_config_t glitch = {.max_glitch_ns = 1000};
    err = pcnt_unit_set_glitch_filter(unit, &glitch);
    if (err != ESP_OK) goto cleanup;
    const pcnt_event_callbacks_t callbacks = {.on_reach = overflow_cb};
    err = pcnt_unit_register_event_callbacks(unit, &callbacks, NULL);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_unit_add_watch_point(unit, COUNT_LIMIT);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_unit_clear_count(unit);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_unit_enable(unit);
    if (err != ESP_OK) goto cleanup;
    err = pcnt_unit_start(unit);
    if (err != ESP_OK) { pcnt_unit_disable(unit); goto cleanup; }
    previous_us = esp_timer_get_time();
    return ESP_OK;
cleanup:
    if (channel) { pcnt_del_channel(channel); channel = NULL; }
    pcnt_del_unit(unit); unit = NULL;
    return err;
}

void flow_sensor_filter_reset(void) { filter.initialized = false; }

esp_err_t flow_sensor_read(const tank_config_t *c, float *rate, double *increment,
                           uint16_t *state)
{
    if (!unit) return ESP_ERR_INVALID_STATE;
    uint64_t count = 0;
    bool stable = false;
    for (unsigned tries = 0; tries < 3; ++tries) {
        uint64_t before, after;
        portENTER_CRITICAL(&count_mux); before = overflow_base; portEXIT_CRITICAL(&count_mux);
        int raw;
        esp_err_t err = pcnt_unit_get_count(unit, &raw);
        if (err != ESP_OK) return err;
        portENTER_CRITICAL(&count_mux); after = overflow_base; portEXIT_CRITICAL(&count_mux);
        count = after + (uint64_t)raw;
        if (before == after && count >= previous) { stable = true; break; }
        /* Hardware may reset at high_limit before its ISR extends the base.
         * Defer a transient backwards snapshot, never clear running pulses. */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    if (!stable) return ESP_ERR_INVALID_STATE;
    const int64_t now = esp_timer_get_time();
    const double dt = (now - previous_us) / 1000000.0;
    if (dt <= 0) return ESP_ERR_INVALID_STATE;
    const float hz = (count - previous) / dt;
    previous = count; previous_us = now;
    float raw_rate = flow_sensor_convert(hz, c);
    /* Rectangular integration of each measured window, before display filtering.
     * Quantization near start/stop is inherent to the window and intercept. */
    *increment = flow_integrate(0, raw_rate, dt);
    *state = raw_rate == 0 ? 1 : (raw_rate < 1 || raw_rate > 30 ? 4 : 2);
    filter.filtered = raw_rate == 0 ? 0 : (filter.initialized ? filter.filtered +
                       c->value[CFG_FLOW_ALPHA] * (raw_rate - filter.filtered) : raw_rate);
    filter.initialized = raw_rate != 0;
    *rate = filter.filtered;
    return ESP_OK;
}
