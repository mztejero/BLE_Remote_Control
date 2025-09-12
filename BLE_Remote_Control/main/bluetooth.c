#include "config.h"
#include "encoder.h"
#include "joystick.h"

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include <string.h>

static const char *ERROR_TAG = "ERRROR_TAG";
static const char *GAP_TAG = "GAP";
static const char *GATT_TAG = "GATT";

static const char *device_name = "RC";

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

static uint16_t current_conn_handle = BLE_HS_CONN_HANDLE_NONE;
uint16_t rc_val_handle;

uint32_t interval_min = 0x00A0;
uint32_t interval_max = 0x00B0;

static const ble_uuid128_t rc_svr_uuid =
    BLE_UUID128_INIT(0x12, 0x34, 0x56, 0x78,
                     0x9a, 0xbc, 0xde, 0xf0,
                     0x12, 0x34, 0x56, 0x78,
                     0x90, 0xab, 0xcd, 0xef);

static const ble_uuid128_t rc_char_uuid =
    BLE_UUID128_INIT(0xab, 0xcd, 0xef, 0x01,
                     0x23, 0x45, 0x67, 0x89,
                     0xab, 0xcd, 0xef, 0x01,
                     0x23, 0x45, 0x67, 0x89);

struct ble_gap_adv_params adv_params;
struct ble_hs_adv_fields adv_fields;

void notify_remote_control_data(uint16_t conn_handle) {

    taskENTER_CRITICAL(&my_spinlock);
    int16_t cl = count_l / 4;
    int16_t cr = count_r / 4;
    taskEXIT_CRITICAL(&my_spinlock);

    uint8_t buf[12];

    memcpy(&buf[0], &js.vxl, 2);
    memcpy(&buf[2], &js.vyl, 2);
    memcpy(&buf[4], &js.vxr, 2);
    memcpy(&buf[6], &js.vyr, 2);
    memcpy(&buf[8], &cl, sizeof(cl));
    memcpy(&buf[10], &cr, sizeof(cr));

    // NimBLE notify
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, sizeof(buf));
    if (om != NULL) {
        ble_gatts_notify_custom(conn_handle, rc_val_handle, om);
    }
}

void notify_task(void *arg) {
    uint16_t conn = current_conn_handle;
    while (conn == current_conn_handle && conn != BLE_HS_CONN_HANDLE_NONE) {
        notify_remote_control_data(conn);
        vTaskDelay(pdMS_TO_TICKS(50)); // 50 Hz
    }
    ESP_LOGI("BLE", "Notify task exited");
    vTaskDelete(NULL);
}


int gap_event_handler(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(GAP_TAG, "Client connected");
                current_conn_handle = event->connect.conn_handle;
                xTaskCreate(notify_task, "notify_task", 4096, NULL, 5, NULL);
            } else {
                ESP_LOGW(GAP_TAG, "Connection failed; restarting advertising");
                ESP_ERROR_CHECK(ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL));
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(GAP_TAG, "Client disconnected; restarting advertising");
            esp_restart();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(GAP_TAG, "Advertisement complete; restarting");
            ESP_ERROR_CHECK(ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL));
            return 0;

        default:
            return 0;
    }
}

static void gap_adv(void) {
    memset(&adv_fields, 0, sizeof adv_fields);

    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    adv_fields.name = (uint8_t *)device_name;
    adv_fields.name_len = strlen(device_name);
    adv_fields.name_is_complete = 1;

    adv_fields.uuids128 = &rc_svr_uuid;
    adv_fields.num_uuids128 = 1;
    adv_fields.uuids128_is_complete = 1;

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ESP_ERROR_CHECK(ble_gap_adv_set_fields(&adv_fields));
    ESP_ERROR_CHECK(ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, gap_event_handler, NULL));
}

void ble_gatt_register(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    char buf[BLE_UUID_STR_LEN];
    switch(ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI(GATT_TAG, "Registered service %s\n", ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf));
            break;
    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI(GATT_TAG, "Registered char %s\n", ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf));
            break;
    case BLE_GATT_REGISTER_OP_DSC:
         ESP_LOGI(GATT_TAG, "Registered descriptor %s\n", ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf));
            break;
    }
}

static int rc_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        const char *msg = "RC ready";
        os_mbuf_append(ctxt->om, msg, strlen(msg));
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        const uint8_t *data = ctxt->om->om_data;
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);

        ESP_LOGI("BLE", "Received write: len=%d, data[0]=%02x", len, data[0]);
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &rc_svr_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                                        {
                                            .uuid = &rc_char_uuid.u,
                                            .access_cb = rc_chr_access_cb,
                                            .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                                            .val_handle = &rc_val_handle
                                        },
                                        {
                                            0
                                        },
                                    },
    },

    {
        0 /* No more services. */
    },
};

int gatt_svr_init(void) {
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    ble_svc_gap_device_name_set(device_name);

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

void adv_on_reset(int reason) {
    ESP_LOGI(GAP_TAG, "Resetting state; reason=%d\n", reason);
}

void adv_on_sync(void) {
    ESP_LOGI(GAP_TAG, "BLE synced");
    gap_adv();
}

void ble_host_task(void *param)
{
     nimble_port_run();
     nimble_port_freertos_deinit();
}

void ble_main_init(void) {
    ESP_ERROR_CHECK(nvs_flash_init());

    esp_err_t err = nimble_port_init();
    if (err != ESP_OK) {
        ESP_LOGE(ERROR_TAG, "nimble_port_init failed: %s", esp_err_to_name(err));
        return;
    }

    ble_hs_cfg.reset_cb = adv_on_reset;
    ble_hs_cfg.sync_cb = adv_on_sync;
    ble_hs_cfg.gatts_register_cb = ble_gatt_register;

    ESP_ERROR_CHECK(gatt_svr_init());
    nimble_port_freertos_init(ble_host_task);
}