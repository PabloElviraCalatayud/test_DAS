#include "ota_manager.h"

#include <string.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"

#define TAG "OTA"

// --------------------
// OTA protocol
// --------------------
#define OTA_CMD_START  0x01
#define OTA_CMD_DATA   0x02
#define OTA_CMD_END    0x03
#define OTA_CMD_ABORT  0x04

typedef struct __attribute__((packed)) {
  uint8_t  cmd;
  uint16_t seq;
  uint16_t len;
  uint8_t  data[];
} ota_pkt_t;

// --------------------
// State
// --------------------
typedef enum {
  OTA_IDLE,
  OTA_RECEIVING,
} ota_state_t;

static ota_state_t s_state = OTA_IDLE;

static esp_ota_handle_t s_ota_handle = 0;
static const esp_partition_t *s_update_partition = NULL;

static uint32_t s_expected_seq = 0;
static uint32_t s_total_written = 0;

// --------------------
// Helpers
// --------------------
static void ota_reset(void) {
  if (s_state == OTA_RECEIVING) {
    esp_ota_abort(s_ota_handle);
  }

  s_state = OTA_IDLE;
  s_ota_handle = 0;
  s_update_partition = NULL;
  s_expected_seq = 0;
  s_total_written = 0;
}

// --------------------
// Public API
// --------------------
void ota_manager_init(void) {
  ota_reset();
}

// --------------------
// Packet handler
// --------------------
void ota_manager_handle_packet(
  const uint8_t *data,
  uint16_t len
) {
  if (len < sizeof(ota_pkt_t)) {
    ESP_LOGW(TAG, "Packet too small");
    return;
  }

  const ota_pkt_t *pkt = (const ota_pkt_t *)data;

  switch (pkt->cmd) {

    // --------------------
    // START
    // --------------------
    case OTA_CMD_START: {
      if (s_state != OTA_IDLE) {
        ESP_LOGW(TAG, "OTA already running");
        return;
      }

      s_update_partition = esp_ota_get_next_update_partition(NULL);
      if (!s_update_partition) {
        ESP_LOGE(TAG, "No OTA partition");
        return;
      }

      esp_err_t err = esp_ota_begin(
        s_update_partition,
        OTA_SIZE_UNKNOWN,
        &s_ota_handle
      );

      if (err != ESP_OK) {
        ESP_LOGE(TAG, "ota_begin failed");
        ota_reset();
        return;
      }

      s_expected_seq = 0;
      s_total_written = 0;
      s_state = OTA_RECEIVING;

      ESP_LOGI(TAG, "OTA START");
      break;
    }

    // --------------------
    // DATA
    // --------------------
    case OTA_CMD_DATA: {
      if (s_state != OTA_RECEIVING) {
        ESP_LOGW(TAG, "DATA while not receiving");
        return;
      }

      if (pkt->seq != s_expected_seq) {
        ESP_LOGE(TAG, "SEQ mismatch exp=%lu got=%u",
          s_expected_seq,
          pkt->seq
        );
        ota_reset();
        return;
      }

      if (pkt->len == 0) {
        ESP_LOGW(TAG, "Empty chunk");
        return;
      }

      esp_err_t err = esp_ota_write(
        s_ota_handle,
        pkt->data,
        pkt->len
      );

      if (err != ESP_OK) {
        ESP_LOGE(TAG, "ota_write failed");
        ota_reset();
        return;
      }

      s_total_written += pkt->len;
      s_expected_seq++;

      break;
    }

    // --------------------
    // END
    // --------------------
    case OTA_CMD_END: {
      if (s_state != OTA_RECEIVING) {
        ESP_LOGW(TAG, "END while not receiving");
        return;
      }

      esp_err_t err = esp_ota_end(s_ota_handle);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "ota_end failed");
        ota_reset();
        return;
      }

      err = esp_ota_set_boot_partition(s_update_partition);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "set_boot_partition failed");
        ota_reset();
        return;
      }

      ESP_LOGI(TAG, "OTA DONE (%lu bytes), rebooting", s_total_written);
      esp_restart();
      break;
    }

    // --------------------
    // ABORT
    // --------------------
    case OTA_CMD_ABORT: {
      ESP_LOGW(TAG, "OTA ABORT");
      ota_reset();
      break;
    }

    default:
      ESP_LOGW(TAG, "Unknown OTA cmd %u", pkt->cmd);
      break;
  }
}

