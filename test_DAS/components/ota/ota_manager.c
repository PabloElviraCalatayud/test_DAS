#include "ota_manager.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"

#include "sensor_manager.h"

#include "system_state.h"
#include "bluetooth.h"

#define OTA_CMD_START  0x01
#define OTA_CMD_DATA   0x02
#define OTA_CMD_END    0x03

#define OTA_ACK        0x80
#define OTA_NACK       0x81

#define OTA_ACK_EVERY_N 4

typedef struct __attribute__((packed)) {
  uint8_t cmd;
  uint16_t seq;
  uint16_t len;
  uint8_t data[];
} ota_pkt_t;

typedef struct {
  uint16_t len;
  uint8_t data[247];
} ota_queue_pkt_t;

typedef struct {
  uint16_t len;
  uint8_t data[247];
} ble_packet_t;

extern QueueHandle_t ble_tx_queue;

static QueueHandle_t ota_queue;
static esp_ota_handle_t ota_handle;
static const esp_partition_t *ota_part;
static uint16_t expected_seq;

static void ota_send(uint8_t cmd, uint16_t seq) {
  ble_packet_t pkt;
  pkt.len = 3;
  pkt.data[0] = cmd;
  pkt.data[1] = seq & 0xff;
  pkt.data[2] = seq >> 8;
  xQueueSend(ble_tx_queue, &pkt, portMAX_DELAY);
}

static void ota_reset(void) {
  ota_handle = 0;
  ota_part = NULL;
  expected_seq = 0;
}

static void ota_task(void *arg) {
  ota_queue_pkt_t qpkt;

  while (1) {
    if (!xQueueReceive(ota_queue, &qpkt, portMAX_DELAY)) {
      continue;
    }

    if (qpkt.len < 5) {
      continue;
    }

    const ota_pkt_t *pkt = (const ota_pkt_t *)qpkt.data;

    switch (pkt->cmd) {

      case OTA_CMD_START:
        if (!system_state_set(SYS_STATE_OTA)) {
          ota_send(OTA_NACK, 0);
          break;
        }

        ota_part = esp_ota_get_next_update_partition(NULL);
        if (!ota_part ||
            esp_ota_begin(ota_part, OTA_SIZE_UNKNOWN, &ota_handle) != ESP_OK) {
          ota_send(OTA_NACK, 0);
          system_state_set(SYS_STATE_RUNNING);
          break;
        }

        expected_seq = 0;
        ota_send(OTA_ACK, 0);
        break;

      case OTA_CMD_DATA:
        if (!system_is_ota() ||
            pkt->seq != expected_seq ||
            pkt->len == 0 ||
            pkt->len > 240 ||
            esp_ota_write(ota_handle, pkt->data, pkt->len) != ESP_OK) {

          ota_send(OTA_NACK, pkt->seq);
          esp_ota_abort(ota_handle);
          ota_reset();
          system_state_set(SYS_STATE_RUNNING);
          break;
        }

        if ((pkt->seq % OTA_ACK_EVERY_N) == 0) {
          ota_send(OTA_ACK, pkt->seq);
        }

        expected_seq++;
        break;

      case OTA_CMD_END:
        ESP_LOGI("OTA", "END received");

        if (!system_is_ota()) {
          ESP_LOGE("OTA", "Not in OTA state");
          ota_send(OTA_NACK, 0);
          break;
        }

        if (esp_ota_end(ota_handle) != ESP_OK) {
          ESP_LOGE("OTA", "esp_ota_end failed");
          ota_send(OTA_NACK, 0);
          system_state_set(SYS_STATE_RUNNING);
          break;
        }

        if (esp_ota_set_boot_partition(ota_part) != ESP_OK) {
          ESP_LOGE("OTA", "set boot partition failed");
          ota_send(OTA_NACK, 0);
          system_state_set(SYS_STATE_RUNNING);
          break;
        }

        ESP_LOGI("OTA", "OTA OK, rebooting");

        ota_send(OTA_ACK, expected_seq);
        vTaskDelay(pdMS_TO_TICKS(200));
        esp_restart();
        break;


      default:
        break;
    }
  }
}

void ota_manager_init(void) {
  ota_reset();
  ota_queue = xQueueCreate(8, sizeof(ota_queue_pkt_t));

  xTaskCreate(
    ota_task,
    "ota_task",
    6144,
    NULL,
    7,
    NULL
  );
}

void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  if (len > 247) {
    return;
  }

  ota_queue_pkt_t pkt;
  pkt.len = len;
  memcpy(pkt.data, data, len);

  xQueueSend(ota_queue, &pkt, 0);
}

