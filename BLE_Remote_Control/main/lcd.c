#include "encoder.h"
#include "joystick.h"

#include "driver/gpio.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_FREQ_HZ 100000
#define LCD_ADDR 0x38

#define LCD_RS (1 << 3)
#define LCD_RW (1 << 2)
#define LCD_EN (1 << 1)
#define LCD_D3 (1 << 0)
#define LCD_D4 (1 << 4)
#define LCD_D5 (1 << 5)
#define LCD_D6 (1 << 6)
#define LCD_D7 (1 << 7)

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

void setup_i2c(void) {
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
}

uint8_t map_nibble(uint8_t nibble) {
    uint8_t out = 0;
    if (nibble & 0x01) out |= LCD_D4;
    if (nibble & 0x02) out |= LCD_D5;
    if (nibble & 0x04) out |= LCD_D6;
    if (nibble & 0x08) out |= LCD_D7;
    return out;
}

void lcd_nibble(uint8_t nibble, bool rs) {
    uint8_t data = map_nibble(nibble);

    if (rs) {
        data |= LCD_RS;
    }

    data &= ~LCD_RW;

    uint8_t with_en = data | LCD_EN;
    i2c_master_transmit(dev_handle, &with_en, 1, -1);

    esp_rom_delay_us(10);

    i2c_master_transmit(dev_handle, &data, 1, -1);
    esp_rom_delay_us(50);
}

void lcd_send_byte(uint8_t value, bool rs) {
    lcd_nibble((value >> 4) & 0x0F, rs);
    lcd_nibble(value & 0x0F, rs);
}

void lcd_print(const char *s) {
        while (*s) {
        lcd_send_byte(*s, 1);
        s++;
    }
}

void lcd_str_and_int(const char *s, int16_t x) {
    char buffer[16];
    sprintf(buffer, "%d", x);  

    lcd_print(s);
    lcd_print(buffer);
}

void lcd_clear(void) {
    lcd_send_byte(0x01, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void lcd_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50));   // wait >40 ms after power up

    // Reset sequence: 3 times 0x03
    lcd_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_nibble(0x03, 0);
    esp_rom_delay_us(150);
    lcd_nibble(0x03, 0);

    // Switch to 4-bit mode
    lcd_nibble(0x02, 0);

    // Function set: 4-bit, 2 line, 5x8 dots
    lcd_send_byte(0x28, 0);
    // Display ON, cursor OFF, blink OFF
    lcd_send_byte(0x0C, 0);
    // Entry mode: increment cursor
    lcd_send_byte(0x06, 0);
    // Clear display
    lcd_send_byte(0x01, 0);
    vTaskDelay(pdMS_TO_TICKS(2));
}

void test_en(void) {
    uint8_t data = LCD_EN;
    while (1) {
        i2c_master_transmit(dev_handle, &data, 1, -1); // EN=1
        vTaskDelay(pdMS_TO_TICKS(200));
        uint8_t zero = 0x00;
        i2c_master_transmit(dev_handle, &zero, 1, -1); // EN=0
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void lcd_task_handler(void *arg) {

    char steering[] = "Steering: ";
    char throttle[] = "Throttle: ";

    int16_t cl_prev = 0;
    int16_t cr_prev = 0;

    bool first_run = true;

    while (1) {

        taskENTER_CRITICAL(&my_spinlock);
        int16_t cl = count_l / 4;
        int16_t cr = count_r / 4;
        taskEXIT_CRITICAL(&my_spinlock);

        if (first_run || cl_prev != cl || cr_prev != cr) {
            lcd_send_byte(0x80 | 0x00, 0);
            lcd_str_and_int(steering, cl);
            lcd_send_byte(0x80 | 0x40, 0);
            lcd_str_and_int(throttle, cr);
        };

        cl_prev = cl;
        cr_prev = cr;
        first_run = false;

        vTaskDelay(pdMS_TO_TICKS(100));
    };
}

void lcd_task(void) {
    xTaskCreate(lcd_task_handler, "LCD", 2048, NULL, 10, NULL);
}