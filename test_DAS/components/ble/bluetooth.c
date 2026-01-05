#include <string.h>
#include <stdbool.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_att.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "bluetooth.h"
#include "ota_manager.h"

static const char *TAG = "BLE";

static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t tx_handle;
static uint8_t own_addr_type;
static bool notify_enabled = false;

static ble_uuid128_t svc_uuid = BLE_UUID128_INIT(
  0x01,0x00,0x00,0x00,
  0xef,0xbe,
  0xad,0xde,
  0xbe,0xef,
  0xde,0xad,
  0xbe,0xef,
  0xde,0xad
);

static ble_uuid128_t chr_tx_uuid = BLE_UUID128_INIT(
  0x02,0x00,0x00,0x00,
  0xef,0xbe,
  0xad,0xde,
  0xbe,0xef,
  0xde,0xad,
  0xbe,0xef,
  0xde,0xad
);

static ble_uuid128_t chr_rx_uuid = BLE_UUID128_INIT(
  0x03,0x00,0x00,0x00,
  0xef,0xbe,
  0xad,0xde,
  0xbe,0xef,
  0xde,0xad,
  0xbe,0xef,
  0xde,0xad
);

static int chr_tx_access_cb(
  uint16_t conn,
  uint16_t attr,
  struct ble_gatt_access_ctxt *ctxt,
  void *arg
) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    notify_enabled = true;
    ESP_LOGI(TAG, "Notify enabled");
  }
  return 0;
}

static int chr_rx_access_cb(
  uint16_t conn,
  uint16_t attr,
  struct ble_gatt_access_ctxt *ctxt,
  void *arg
) {
  ota_manager_handle_packet(
    ctxt->om->om_data,
    ctxt->om->om_len
  );
  return 0;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
  {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = &svc_uuid.u,
    .characteristics = (struct ble_gatt_chr_def[]) {
      {
        .uuid = &chr_tx_uuid.u,
        .access_cb = chr_tx_access_cb,
        .flags = BLE_GATT_CHR_F_NOTIFY,
        .val_handle = &tx_handle,
      },
      {
        .uuid = &chr_rx_uuid.u,
        .access_cb = chr_rx_access_cb,
        .flags = BLE_GATT_CHR_F_WRITE |
                 BLE_GATT_CHR_F_WRITE_NO_RSP,
      },
      { 0 }
    },
  },
  { 0 }
};

static void ble_advertise(void);

static int gap_event_cb(
  struct ble_gap_event *event,
  void *arg
) {
  switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
      if (event->connect.status == 0) {
        conn_handle = event->connect.conn_handle;
        notify_enabled = false;
        ESP_LOGI(TAG, "Connected");
      } else {
        ble_advertise();
      }
      break;

    case BLE_GAP_EVENT_DISCONNECT:
      conn_handle = BLE_HS_CONN_HANDLE_NONE;
      notify_enabled = false;
      ESP_LOGI(TAG, "Disconnected");
      ble_advertise();
      break;

    case BLE_GAP_EVENT_SUBSCRIBE:
      if (event->subscribe.attr_handle == tx_handle) {
        notify_enabled = event->subscribe.cur_notify;
        ESP_LOGI(
          TAG,
          "Notify %s",
          notify_enabled ? "ENABLED" : "DISABLED"
        );
      }
      break;

    case BLE_GAP_EVENT_MTU:
      ESP_LOGI(TAG, "MTU %d", event->mtu.value);
      break;

    default:
      break;
  }

  return 0;
}

static void ble_advertise(void) {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;

  memset(&fields, 0, sizeof(fields));
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  fields.name = (uint8_t *)"ESP32_NIMBLE";
  fields.name_len = strlen((char *)fields.name);
  fields.name_is_complete = 1;

  ble_gap_adv_set_fields(&fields);

  memset(&adv_params, 0, sizeof(adv_params));
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

  ble_gap_adv_start(
    own_addr_type,
    NULL,
    BLE_HS_FOREVER,
    &adv_params,
    gap_event_cb,
    NULL
  );
}

static void ble_on_sync(void) {
  ble_hs_id_infer_auto(0, &own_addr_type);
  ble_advertise();
}

static void ble_host_task(void *param) {
  nimble_port_run();
  nimble_port_freertos_deinit();
}

void bluetooth_init(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  esp_nimble_hci_init();
  nimble_port_init();

  ble_att_set_preferred_mtu(247);

  ble_hs_cfg.sync_cb = ble_on_sync;

  ble_gatts_count_cfg(gatt_svcs);
  ble_gatts_add_svcs(gatt_svcs);

  ble_svc_gap_init();
  ble_svc_gatt_init();

  nimble_port_freertos_init(ble_host_task);
}


int bluetooth_notify(const uint8_t *data, uint16_t len) {
  if (conn_handle == BLE_HS_CONN_HANDLE_NONE ||
      tx_handle == 0 ||
      !notify_enabled) {
    return -1;
  }

  struct os_mbuf *om = ble_hs_mbuf_from_flat(data, len);
  if (!om) {
    return -1;
  }

  return ble_gatts_notify_custom(
    conn_handle,
    tx_handle,
    om
  );
}

