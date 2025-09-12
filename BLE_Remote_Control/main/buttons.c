#include "config.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"

// Read Pins
void read_pins(void) {

    int sw_el = gpio_get_level(pins.sw_enc_l);
    int sw_er = gpio_get_level(pins.sw_enc_r);
    int sw_jl = gpio_get_level(pins.sw_js_l);
    int sw_jr = gpio_get_level(pins.sw_js_r);

    printf("SW ENC L: %d\tSW ENC R: %d\tSW JS L: %d\tSW JS R: %d\n", sw_el, sw_er, sw_jl, sw_jr);
}