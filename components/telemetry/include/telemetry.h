#pragma once
#include <stdbool.h>
#include <stdint.h>

#define TANK_REPORT_MIN_S 30U
#define TANK_REPORT_MAX_S 600U

/* Configuration wire IDs equal array indices in the private cluster. */
enum { CFG_ACQUIRE_MS, CFG_PUBLISH_S, CFG_SHUNT_OHM, CFG_CURRENT_MIN,
       CFG_CURRENT_MAX, CFG_HEIGHT_SPAN, CFG_HEIGHT_MIN, CFG_HEIGHT_MAX,
       CFG_LEVEL_ALPHA, CFG_FLOW_SLOPE, CFG_FLOW_INTERCEPT, CFG_FLOW_THRESHOLD,
       CFG_FLOW_ALPHA, CFG_FLOW_ENABLED, CFG_COUNT };
typedef struct { float value[CFG_COUNT]; } tank_config_t;
typedef struct {
    float value[6]; /* m, %, L/min, L, mA, V */
    uint16_t level_state; /* 1 ok, 2 adc_error, 3 under_range, 4 over_range */
    uint16_t flow_state;  /* 1 no_flow, 2 flowing, 3 pcnt_error, 4 out_of_range, 5 disabled */
    int64_t timestamp_us;
} tank_sample_t;

void tank_config_defaults(tank_config_t *config);
bool tank_config_valid(const tank_config_t *config);
void tank_state_init(const tank_config_t *config);
void tank_config_get(tank_config_t *config, uint32_t *revision);
bool tank_config_set(unsigned id, float value);
void tank_request_reset(void);
bool tank_take_reset(void);
