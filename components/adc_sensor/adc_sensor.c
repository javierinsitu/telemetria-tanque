#include "adc_sensor.h"
#include "driver/i2c_master.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum {
    ADC_REG_CONVERSION = 0,
    ADC_REG_CONFIG = 1,
    /* OS=1, A0/GND, PGA +/-4.096 V, single shot, 128 SPS,
     * comparator disabled. ADS1115 datasheet, config register table. */
    ADC_CONFIG = 0xc383,
    I2C_TIMEOUT_MS = 20,
    CONVERSION_TIMEOUT_US = 40000,
};

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t device;

static esp_err_t read_register(uint8_t reg, uint16_t *value)
{
    uint8_t bytes[2];
    esp_err_t err = i2c_master_transmit_receive(device, &reg, 1,
                                              bytes, sizeof(bytes), I2C_TIMEOUT_MS);
    if (err == ESP_OK) {
        *value = ((uint16_t)bytes[0] << 8) | bytes[1];
    }
    return err;
}

esp_err_t adc_sensor_init(uint8_t address)
{
    if (device != NULL) return ESP_ERR_INVALID_STATE;
    if (address < 0x48 || address > 0x4b) return ESP_ERR_INVALID_ARG;
    const i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_3,
        .scl_io_num = GPIO_NUM_2,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        /* External pullups to 3.3 V required. */
        .flags.enable_internal_pullup = false,
    };
    esp_err_t err = i2c_new_master_bus(&bus_config, &bus);
    if (err != ESP_OK) return err;
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 100000,
    };
    err = i2c_master_bus_add_device(bus, &config, &device);
    if (err != ESP_OK) {
        i2c_del_master_bus(bus);
        bus = NULL;
    }
    /* Absence on the bus is handled by read(), allowing hot recovery without
     * allocation/reinitialization on each failed acquisition. */
    return err;
}

esp_err_t adc_sensor_read(adc_sample_t *sample)
{
    if (sample == NULL) return ESP_ERR_INVALID_ARG;
    if (device == NULL) return ESP_ERR_INVALID_STATE;
    const uint8_t command[] = {ADC_REG_CONFIG, ADC_CONFIG >> 8, ADC_CONFIG & 0xff};
    esp_err_t err = i2c_master_transmit(device, command, sizeof(command), I2C_TIMEOUT_MS);
    if (err != ESP_OK) return err;

    const int64_t deadline = esp_timer_get_time() + CONVERSION_TIMEOUT_US;
    uint16_t config;
    do {
        vTaskDelay(pdMS_TO_TICKS(2));
        err = read_register(ADC_REG_CONFIG, &config);
        if (err != ESP_OK) return err;
        /* Reject changed config (OS is read-only conversion status here). */
        if ((config & 0x7fff) != (ADC_CONFIG & 0x7fff)) return ESP_ERR_INVALID_RESPONSE;
        if (esp_timer_get_time() >= deadline) return ESP_ERR_TIMEOUT;
    } while (!(config & 0x8000));

    uint16_t bits;
    err = read_register(ADC_REG_CONVERSION, &bits);
    if (err != ESP_OK) return err;
    const int16_t raw = (int16_t)bits;
    const float voltage = raw * 0.000125f;
    /* Do not clamp invalid data into a plausible reading. Slight negative
     * offset at grounded A0 is allowed. Electrical protection is hardware. */
    if (raw == INT16_MAX || raw == INT16_MIN || voltage < -0.01f || voltage > 3.3f) {
        return ESP_ERR_INVALID_RESPONSE;
    }
    *sample = (adc_sample_t){.raw = raw, .voltage_v = voltage,
                             .timestamp_us = esp_timer_get_time()};
    return ESP_OK;
}
