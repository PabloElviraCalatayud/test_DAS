#include "ota_manager.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_ota_ops.h"
#include "esp_system.h"

#define OTA_CMD_START  0x01
#define OTA_CMD_DATA   0x02
#define OTA_CMD_END    0x03

#define OTA_ACK        0x80
#define OTA_NACK       0x81

#define OTA_ACK_EVERY_N 4

typedef struct __attribute__((packed)) {
  uint8_t  cmd;
  uint16_t seq;
  uint16_t len;
  uint8_t  data[];
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

typedef enum {
  OTA_IDLE,
  OTA_RECEIVING,
} ota_state_t;

static QueueHandle_t ota_queue;
static ota_state_t s_state;
static esp_ota_handle_t s_handle;
static const esp_partition_t *s_part;
static uint16_t s_expected_seq;

static void ota_send_ack(uint8_t cmd, uint16_t seq) {
  ble_packet_t pkt;

  pkt.len = 3;
  pkt.data[0] = cmd;
  pkt.data[1] = seq & 0xff;
  pkt.data[2] = seq >> 8;

  xQueueSend(ble_tx_queue, &pkt, portMAX_DELAY);
}

static void ota_reset(void) {
  s_state = OTA_IDLE;
  s_handle = 0;
  s_part = NULL;
  s_expected_seq = 0;
}

static void ota_task(void *arg) {
  ota_queue_pkt_t qpkt;

  while (1) {
    if (!xQueueReceive(ota_queue, &qpkt, portMAX_DELAY)) {
      continue;
    }

    const ota_pkt_t *pkt = (const ota_pkt_t *)qpkt.data;

    switch (pkt->cmd) {

      case OTA_CMD_START: {
        if (s_state != OTA_IDLE) {
          ota_send_ack(OTA_NACK, 0);
          break;
        }

        s_part = esp_ota_get_next_update_partition(NULL);
        if (!s_part ||
            esp_ota_begin(s_part, OTA_SIZE_UNKNOWN, &s_handle) != ESP_OK) {
          ota_send_ack(OTA_NACK, 0);
          ota_reset();
          break;
        }

        s_state = OTA_RECEIVING;
        s_expected_seq = 0;
        ota_send_ack(OTA_ACK, 0);
        break;
      }

      case OTA_CMD_DATA: {
        if (s_state != OTA_RECEIVING ||
          pkt->seq != s_expected_seq ||
          pkt->len == 0 ||
          pkt->len > 240 ||
        esp_ota_write(s_handle, pkt->data, pkt->len) != ESP_OK) {
        taskYIELD();
        ota_send_ack(OTA_NACK, pkt->seq);
        esp_ota_abort(s_handle);
        ota_reset();
      break;
      }

  if ((pkt->seq % OTA_ACK_EVERY_N) == 0) {
    ota_send_ack(OTA_ACK, pkt->seq);
  }

  s_expected_seq++;
  break;
}

      case OTA_CMD_END: {
        if (s_state != OTA_RECEIVING ||
            esp_ota_end(s_handle) != ESP_OK ||
            esp_ota_set_boot_partition(s_part) != ESP_OK) {

          ota_send_ack(OTA_NACK, 0);
          ota_reset();
          break;
        }

        ota_send_ack(OTA_ACK, s_expected_seq);
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
        break;
      }

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
    6,
    NULL
  );
}

bool ota_manager_is_active(void) {
  return s_state == OTA_RECEIVING;
}

void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  ota_queue_pkt_t pkt;

  if (len > sizeof(pkt.data)) {
    return;
  }

  pkt.len = len;
  memcpy(pkt.data, data, len);

  xQueueSend(ota_queue, &pkt, 0);
}

