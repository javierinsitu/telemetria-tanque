#include "level_sensor.h"
#include <math.h>

void level_sensor_process(level_filter_t *f, const tank_config_t *c,
                          bool adc_ok, float voltage, tank_sample_t *s)
{
    s->value[0] = s->value[1] = s->value[4] = s->value[5] = NAN;
    s->level_state = 2;
    if (!adc_ok || !isfinite(voltage)) { f->initialized = false; return; }
    const float *v = c->value;
    float current = voltage * 1000 / v[CFG_SHUNT_OHM];
    s->value[4] = current;
    s->value[5] = voltage;
    /* 0.4 mA tolerance is a configurable-calibration margin, not a claim that
     * HY-5000 implements NAMUR fault signaling. Preserve raw electrical data. */
    if (current < v[CFG_CURRENT_MIN] - 0.4f || current > v[CFG_CURRENT_MAX] + 0.4f) {
        s->level_state = current < v[CFG_CURRENT_MIN] ? 3 : 4;
        f->initialized = false;
        return;
    }
    float height = (current - v[CFG_CURRENT_MIN]) * v[CFG_HEIGHT_SPAN] /
                   (v[CFG_CURRENT_MAX] - v[CFG_CURRENT_MIN]);
    height = fminf(v[CFG_HEIGHT_SPAN], fmaxf(0, height));
    f->filtered_m = f->initialized ? f->filtered_m + v[CFG_LEVEL_ALPHA] *
                    (height - f->filtered_m) : height;
    f->initialized = true;
    s->value[0] = f->filtered_m;
    s->value[1] = fminf(100, fmaxf(0, 100 * (f->filtered_m - v[CFG_HEIGHT_MIN]) /
                                      (v[CFG_HEIGHT_MAX] - v[CFG_HEIGHT_MIN])));
    s->level_state = 1;
}
