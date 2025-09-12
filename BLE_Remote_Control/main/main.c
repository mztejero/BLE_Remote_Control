// idf.py -p /dev/cu.usbserial-1410 -b 115200 flash monitor
#include "config.h"
#include "encoder.h"
#include "joystick.h"
#include "buttons.h"
#include "bluetooth.h"
#include "lcd.h"

void app_main(void) {
    enc_table();
    setup_gpio();
    setup_adc();
    setup_queues();
    setup_isr();
    initialize_joystick(&js_l, &js_r, &params);
    ble_main_init();
    setup_i2c();
    lcd_init();
    lcd_clear();
    lcd_task();
    while (1) {
        read_joystick(&js_l, &js_r, &params);
        printf("JS vxl: %d, vyl: %d, vxr: %d, vyr: %d | Enc L: %d, R: %d\n",
               js.vxl, js.vyl, js.vxr, js.vyr, count_l/4, count_r/4);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}