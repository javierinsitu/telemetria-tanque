#include "telemetry.h"
#include <math.h>
#include "sdkconfig.h"

void tank_config_defaults(tank_config_t *c)
{
    *c = (tank_config_t){.value = {CONFIG_TANK_ACQUISITION_MS, TANK_REPORT_MIN_S, 150, 4, 20,
                                  5, 0, 5, 0.2f, 8.1f, -3, 0.5f, 0.3f, 0}};
}

bool tank_config_valid(const tank_config_t *c)
{
    for (unsigned i = 0; i < CFG_COUNT; ++i) if (!isfinite(c->value[i])) return false;
    const float *v = c->value;
    return v[CFG_ACQUIRE_MS] >= 100 && v[CFG_ACQUIRE_MS] <= 10000 &&
           floorf(v[CFG_ACQUIRE_MS]) == v[CFG_ACQUIRE_MS] &&
           v[CFG_PUBLISH_S] >= TANK_REPORT_MIN_S && v[CFG_PUBLISH_S] <= TANK_REPORT_MAX_S &&
           floorf(v[CFG_PUBLISH_S]) == v[CFG_PUBLISH_S] &&
           v[CFG_SHUNT_OHM] >= 100 && v[CFG_SHUNT_OHM] <= 200 &&
           v[CFG_CURRENT_MIN] >= 2 && v[CFG_CURRENT_MIN] <= 6 &&
           v[CFG_CURRENT_MAX] >= 15 && v[CFG_CURRENT_MAX] <= 22 &&
           v[CFG_CURRENT_MAX] > v[CFG_CURRENT_MIN] &&
           v[CFG_HEIGHT_SPAN] > 0 && v[CFG_HEIGHT_SPAN] <= 10 &&
           v[CFG_HEIGHT_MIN] >= 0 && v[CFG_HEIGHT_MAX] <= 10 &&
           v[CFG_HEIGHT_MAX] > v[CFG_HEIGHT_MIN] &&
           v[CFG_LEVEL_ALPHA] > 0 && v[CFG_LEVEL_ALPHA] <= 1 &&
           v[CFG_FLOW_SLOPE] >= 0.1f && v[CFG_FLOW_SLOPE] <= 100 &&
           v[CFG_FLOW_INTERCEPT] >= -100 && v[CFG_FLOW_INTERCEPT] <= 100 &&
           v[CFG_FLOW_THRESHOLD] >= 0 && v[CFG_FLOW_THRESHOLD] <= 30 &&
           v[CFG_FLOW_ALPHA] > 0 && v[CFG_FLOW_ALPHA] <= 1 &&
           (v[CFG_FLOW_ENABLED] == 0 || v[CFG_FLOW_ENABLED] == 1);
}
