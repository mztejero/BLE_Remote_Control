#ifndef ENCODER_H
#define ENCODER_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern volatile int count_l;
extern volatile int count_r;

extern QueueHandle_t queue_l;
extern QueueHandle_t queue_r;

void enc_table(void);
void setup_queues(void);
void setup_isr(void);

#endif