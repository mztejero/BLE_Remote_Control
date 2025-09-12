#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

typedef struct {
    gpio_num_t clk_l;
    gpio_num_t dt_l;
    gpio_num_t sw_enc_l;
    gpio_num_t clk_r;
    gpio_num_t dt_r;
    gpio_num_t sw_enc_r;
    adc_channel_t vx_l;
    adc_channel_t vy_l;
    gpio_num_t sw_js_l;
    adc_channel_t vx_r;
    adc_channel_t vy_r;
    gpio_num_t sw_js_r;
} PinConfig;

extern const PinConfig pins;

void setup_gpio(void);

#endif