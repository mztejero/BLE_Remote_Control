#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
// #include "bleprph.h"
#include <string.h>

void notify_task(void *arg);
void ble_main_init(void);

#endif