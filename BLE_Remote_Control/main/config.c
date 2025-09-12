#include "config.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const PinConfig pins = {
    .clk_l = GPIO_NUM_19,
    .dt_l = GPIO_NUM_18,
    .sw_enc_l = GPIO_NUM_5,
    .clk_r = GPIO_NUM_17,
    .dt_r = GPIO_NUM_16,
    .sw_enc_r = GPIO_NUM_4,
    .vx_l = ADC_CHANNEL_6,
    .vy_l = ADC_CHANNEL_7,
    .sw_js_l = GPIO_NUM_32,
    .vx_r = ADC_CHANNEL_8,
    .vy_r = ADC_CHANNEL_9,
    .sw_js_r = GPIO_NUM_33
};

// GPIO Setup
void setup_gpio(void) {
    // Encoder Terminals (CLK and DT)
    gpio_config_t conf_clk_dt = {
        .pin_bit_mask = (1ULL << pins.clk_l) |
                        (1ULL << pins.clk_r) |
                        (1ULL << pins.dt_l) |
                        (1ULL << pins.dt_r),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&conf_clk_dt);

    // Buttons (SW)
    gpio_config_t conf_sw = {
        .pin_bit_mask = (1ULL << pins.sw_enc_l) |
                        (1ULL << pins.sw_enc_r) |
                        (1ULL << pins.sw_js_l) |
                        (1ULL << pins.sw_js_r),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&conf_sw);
}