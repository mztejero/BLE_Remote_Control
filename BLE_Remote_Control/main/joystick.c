#include "config.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

// Helper Functions
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Define Handles
adc_oneshot_unit_handle_t adc1_handle;

// Structs
typedef struct {
    int16_t adc_max;
    int16_t calibration_samples;
} Parameters;

typedef struct {
    int16_t vx_offset;
    int16_t vy_offset;
    int16_t vx_range;
    int16_t vy_range;
} Joystick;

typedef struct {
    int16_t vxl;
    int16_t vyl;
    int16_t vxr;
    int16_t vyr;
} JoystickVals;

// Initialize Structs
Parameters params = {
    .adc_max = 4095,
    .calibration_samples = 100
};

Joystick js_l;
Joystick js_r;

JoystickVals js = {
    .vxl = 0,
    .vyl = 0,
    .vxr = 0,
    .vyr = 0
};

// ADC Setup
void setup_adc(void) {
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1
    };
    adc_oneshot_new_unit(&init_cfg, &adc1_handle);

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12
    };

    adc_oneshot_config_channel(adc1_handle, pins.vx_l, &chan_cfg);
    adc_oneshot_config_channel(adc1_handle, pins.vy_l, &chan_cfg);

    adc_oneshot_config_channel(adc1_handle, pins.vx_r, &chan_cfg);
    adc_oneshot_config_channel(adc1_handle, pins.vy_r, &chan_cfg);
}

// Initialize Joystick
void initialize_joystick(Joystick *js_l, Joystick *js_r, Parameters *params) {
    printf("Initializing Joystick\n");
    long vxl_sum = 0;
    long vyl_sum = 0;
    long vxr_sum = 0;
    long vyr_sum = 0;
    for (int i = 0; i < params->calibration_samples; i++) {
        int sample;
        adc_oneshot_read(adc1_handle, pins.vx_l, &sample);
        vxl_sum += sample;
        adc_oneshot_read(adc1_handle, pins.vy_l, &sample);
        vyl_sum += sample;
        adc_oneshot_read(adc1_handle, pins.vx_r, &sample);
        vxr_sum += sample;
        adc_oneshot_read(adc1_handle, pins.vy_r, &sample);
        vyr_sum += sample;

        vTaskDelay(pdMS_TO_TICKS(10));
    } 
    js_l->vx_offset = (vxl_sum / params->calibration_samples);
    js_l->vy_offset = (vyl_sum / params->calibration_samples);
    js_r->vx_offset = (vxr_sum / params->calibration_samples);
    js_r->vy_offset = (vyr_sum / params->calibration_samples);

    js_l->vx_range = MIN(js_l->vx_offset, params->adc_max - js_l->vx_offset);
    js_l->vy_range = MIN(js_l->vy_offset, params->adc_max - js_l->vy_offset);
    js_r->vx_range = MIN(js_r->vx_offset, params->adc_max - js_r->vx_offset);
    js_r->vy_range = MIN(js_r->vy_offset, params->adc_max - js_r->vy_offset);
    printf("Initialized Offsets = vxl: %d, vyl: %d, vxr: %d, vyr: %d\n", js_l->vx_offset, js_l->vy_offset, js_r->vx_offset, js_r->vy_offset);
    printf("Initialized Ranges = vxl: %d, vyl: %d, vxr: %d, vyr: %d\n", js_l->vx_range, js_l->vy_range, js_r->vx_range, js_r->vy_range);
}

// Read Joystick
void read_joystick(Joystick *js_l, Joystick *js_r, Parameters *params) {
    int vxl_raw = 0;
    int vyl_raw = 0;
    int vxr_raw = 0;
    int vyr_raw = 0;
    
    adc_oneshot_read(adc1_handle, pins.vx_l, &vxl_raw);
    adc_oneshot_read(adc1_handle, pins.vy_l, &vyl_raw);
    adc_oneshot_read(adc1_handle, pins.vx_r, &vxr_raw);
    adc_oneshot_read(adc1_handle, pins.vy_r, &vyr_raw);

    int delta_vxl = (vxl_raw - js_l->vx_offset);
    int delta_vyl = (vyl_raw - js_l->vy_offset);
    int delta_vxr = (vxr_raw - js_r->vx_offset);
    int delta_vyr = (vyr_raw - js_r->vy_offset);

    if (delta_vxl > 0) {
        js.vxl = 100 * delta_vxl / js_l->vx_range;
        js.vxl = MAX(-100, MIN(100, js.vxl));
    }
    else if (delta_vxl < 0) {
        js.vxl = 100 * delta_vxl / js_l->vx_range;
        js.vxl = MAX(-100, MIN(100, js.vxl));
    }
    if (delta_vyl > 0) {
        js.vyl = 100 * delta_vyl / js_l->vy_range;
        js.vyl = MAX(-100, MIN(100, js.vyl));
    }
    else if (delta_vyl < 0) {
        js.vyl = 100 * delta_vyl / js_l->vy_range;
        js.vyl = MAX(-100, MIN(100, js.vyl));
    }

    if (delta_vxr > 0) {
        js.vxr = 100 * delta_vxr / js_r->vx_range;
        js.vxr = MAX(-100, MIN(100, js.vxr));
    }
    else if (delta_vxr < 0) {
        js.vxr = 100 * delta_vxr / js_r->vx_range;
        js.vxr = MAX(-100, MIN(100, js.vxr));
    }
    if (delta_vyr > 0) {
        js.vyr = 100 * delta_vyr / js_r->vy_range;
        js.vyr = MAX(-100, MIN(100, js.vyr));
    }
    else if (delta_vyr < 0) {
        js.vyr = 100 * delta_vyr / js_r->vy_range;
        js.vyr = MAX(-100, MIN(100, js.vyr));
    }

    // printf("Left Joystick: vx=%ld vy=%ld | Right Joystick: vx=%ld vy=%ld\n", js.vxl, js.vyl, js.vxr, js.vyr);
}