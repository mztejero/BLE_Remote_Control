#ifndef LCD_H
#define LCD_H

#include "driver/gpio.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"

void setup_i2c(void);
void lcd_init(void);
void lcd_clear(void);
void lcd_task(void);

#endif