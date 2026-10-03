#include "flow_sensor.h"
#include <math.h>
float flow_sensor_convert(float hz, const tank_config_t *c)
{
    if (!isfinite(hz) || hz <= 0) return 0;
    float rate = fmaxf(0, (hz - c->value[CFG_FLOW_INTERCEPT]) / c->value[CFG_FLOW_SLOPE]);
    return rate >= c->value[CFG_FLOW_THRESHOLD] ? rate : 0;
}
double flow_integrate(double liters, float rate, double seconds)
{
    return isfinite(rate) && rate >= 0 && isfinite(seconds) && seconds > 0 ?
           liters + rate * seconds / 60.0 : liters;
}
