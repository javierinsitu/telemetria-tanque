#include "telemetry.h"
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "sdkconfig.h"

static StaticSemaphore_t mutex_buffer;
static SemaphoreHandle_t mutex;
static tank_config_t current;
static uint32_t generation;
static bool reset_requested;

void tank_state_init(const tank_config_t *c)
{
    mutex = xSemaphoreCreateMutexStatic(&mutex_buffer);
    current = *c;
}

void tank_config_get(tank_config_t *c, uint32_t *rev)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    *c = current;
    if (rev) *rev = generation;
    xSemaphoreGive(mutex);
}

bool tank_config_set(unsigned id, float value)
{
    if (id >= CFG_COUNT) return false;
    xSemaphoreTake(mutex, portMAX_DELAY);
    tank_config_t candidate = current;
    candidate.value[id] = value;
    bool valid = tank_config_valid(&candidate);
    if (valid && memcmp(&current, &candidate, sizeof(current))) {
        current = candidate;
        ++generation;
    }
    xSemaphoreGive(mutex);
    return valid;
}

void tank_request_reset(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    reset_requested = true;
    xSemaphoreGive(mutex);
}

bool tank_take_reset(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    bool pending = reset_requested;
    reset_requested = false;
    xSemaphoreGive(mutex);
    return pending;
}
