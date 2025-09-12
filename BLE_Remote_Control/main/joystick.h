#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

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

extern Parameters params;

extern Joystick js_l;
extern Joystick js_r;

extern JoystickVals js;

void setup_adc(void);
void initialize_joystick(Joystick *js_l, Joystick *js_r, Parameters *params);
void read_joystick(Joystick *js_l, Joystick *js_r, Parameters *params);

#endif