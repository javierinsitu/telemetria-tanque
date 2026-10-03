#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "telemetry.h"
#include "level_sensor.h"
#include "flow_sensor.h"
static void near(double a, double b) { assert(fabs(a-b) < 0.0001); }
#ifdef ESP_PLATFORM
void tank_run_self_tests(void)
#else
int main(void)
#endif
{
    tank_config_t c; tank_config_defaults(&c); assert(tank_config_valid(&c));
    level_filter_t f = {0}; tank_sample_t s;
    level_sensor_process(&f, &c, true, 0.6, &s); near(s.value[0],0); near(s.value[1],0); near(s.value[4],4);
    f.initialized = false;
    level_sensor_process(&f, &c, true, 1.8, &s); near(s.value[0],2.5); near(s.value[1],50);
    f.initialized = false;
    level_sensor_process(&f, &c, true, 3, &s); near(s.value[0],5); near(s.value[1],100);
    level_sensor_process(&f, &c, false, 3, &s); assert(s.level_state==2 && isnan(s.value[0]));
    level_sensor_process(&f, &c, true, 0, &s); assert(s.level_state==3 && isnan(s.value[0]));
    level_sensor_process(&f, &c, true, 3.2, &s); assert(s.level_state==4 && isnan(s.value[1]));
    c.value[CFG_CURRENT_MIN]=5; c.value[CFG_CURRENT_MAX]=19;
    level_sensor_process(&f, &c, true,1.8,&s); near(s.value[0],2.5);
    c.value[CFG_HEIGHT_MIN]=1; c.value[CFG_HEIGHT_MAX]=4;
    f.initialized=false; level_sensor_process(&f,&c,true,1.8,&s); near(s.value[1],50);
    tank_config_defaults(&c);
    near(flow_sensor_convert(0,&c),0); near(flow_sensor_convert(5.1,&c),1);
    near(flow_sensor_convert(78,&c),10); near(flow_sensor_convert(240,&c),30);
    near(flow_integrate(12,10,60),22);
    double total=0; for(int i=0;i<1000;i++) total=flow_integrate(total,flow_sensor_convert(78,&c),0.06);
    near(total,10); near(flow_integrate(total,0,120),10);
    c.value[CFG_FLOW_INTERCEPT]=3; near(flow_sensor_convert(1,&c),0);
    c.value[CFG_FLOW_SLOPE]=0; assert(!tank_config_valid(&c));
    tank_config_defaults(&c); c.value[CFG_HEIGHT_MAX]=0; assert(!tank_config_valid(&c));
    tank_config_defaults(&c); c.value[CFG_LEVEL_ALPHA]=NAN; assert(!tank_config_valid(&c));
    tank_config_defaults(&c); c.value[CFG_ACQUIRE_MS]=1000.5; assert(!tank_config_valid(&c));
    puts("OK: level endpoints, calibration, invalid/stale sensor data, flow intercept, zero, integration, configuration validation");
}
