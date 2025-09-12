#include "config.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Global Variables
volatile int count_l = 1500 * 4;
volatile int count_r = 1500 * 4;

// Define Handles
QueueHandle_t queue_l;
QueueHandle_t queue_r;

// Encoder Lookup Table
int8_t enc[256] = {0};
void enc_table(void) {
    enc[0b0001] = -1;
    enc[0b0111] = -1;
    enc[0b1110] = -1;
    enc[0b1000] = -1;

    enc[0b0010] = 1;
    enc[0b0100] = 1;
    enc[0b1101] = 1;
    enc[0b1011] = 1;
}

// ISR's
void IRAM_ATTR left_isr_handler(void *arg) {
    static uint8_t state_l = 0b1111;

    uint8_t clk = gpio_get_level(pins.clk_l);
    uint8_t dt = gpio_get_level(pins.dt_l);

    uint8_t curr = (clk << 1) | dt;
    state_l = (state_l << 2) | curr;
    uint8_t state = state_l;

    BaseType_t xHPW = pdFALSE;
    xQueueSendFromISR(queue_l, &state, (TickType_t)0);
    if (xHPW) portYIELD_FROM_ISR();
}

void IRAM_ATTR right_isr_handler(void *arg) {
    static uint8_t state_r = 0b1111;

    uint8_t clk = gpio_get_level(pins.clk_r);
    uint8_t dt = gpio_get_level(pins.dt_r);
    
    uint8_t curr = (clk << 1) | dt;
    state_r = (state_r << 2) | curr;
    uint8_t state = state_r;

    BaseType_t xHPW = pdFALSE;
    xQueueSendFromISR(queue_r, &state, (TickType_t)0);
    if (xHPW) portYIELD_FROM_ISR(); 
}

static inline void print_u8_bin(uint8_t v)
{
    for (int i = 7; i >= 0; --i) {
        putchar((v & (1 << i)) ? '1' : '0');
    }
}

void left_isr_task(void *arg) {
    uint8_t state;
    while (1) {
        if (xQueueReceive(queue_l, &state, portMAX_DELAY) == pdTRUE) {
            count_l += enc[state & 0x0F];
        }
    }
}

void right_isr_task(void *arg) {
    uint8_t state;
    while (1) {
        if (xQueueReceive(queue_r, &state, portMAX_DELAY) == pdTRUE) {
            count_r += enc[state & 0x0F];
        }
    }
}

// Queue Setup
void setup_queues(void) {
    queue_l = xQueueCreate(64, sizeof(uint8_t));
    queue_r = xQueueCreate(64, sizeof(uint8_t));
    if (!queue_l || !queue_r) {
        printf("Failed to create queues\n");
    }

    if (xTaskCreate(left_isr_task, "left_isr_task", 2048, NULL, 10, NULL) != pdPASS) {
        printf("Failed to create left_isr_task\n");
    }
    if (xTaskCreate(right_isr_task, "right_isr_task", 2048, NULL, 10, NULL) != pdPASS) {
        printf("Failed to create right_isr_task\n");
    }

}

// ISR Setup
void setup_isr(void) {
    gpio_install_isr_service(0);

    gpio_isr_handler_add(pins.clk_l, left_isr_handler, (void* )pins.clk_l);
    gpio_isr_handler_add(pins.dt_l, left_isr_handler, (void* )pins.dt_l);
    gpio_isr_handler_add(pins.clk_r, right_isr_handler, (void* )pins.clk_r);
    gpio_isr_handler_add(pins.dt_r, right_isr_handler, (void* )pins.dt_r);
}