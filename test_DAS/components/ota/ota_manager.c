#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "bluetooth.h"

#define OTA_TAG "OTA"

#define OTA_CMD_START  0x01
#define OTA_CMD_DATA   0x02
#define OTA_CMD_END    0x03
#define OTA_ACK        0x80
#define OTA_NACK       0x81

typedef struct {
  uint8_t cmd;
  uint16_t seq;
  uint16_t len;
  uint8_t data[240];
} ota_packet_t;

static QueueHandle_t ota_queue;
static esp_ota_handle_t ota_handle;
static const esp_partition_t *ota_partition;
static uint16_t expected_seq;
static bool ota_active;

static void ota_reset(void) {
  ota_active = false;
  expected_seq = 0;
  ota_partition = NULL;
  ota_handle = 0;
}

static void ota_send_resp(uint8_t cmd, uint16_t seq) {
  uint8_t buf[5];
  buf[0] = cmd;
  buf[1] = seq & 0xff;
  buf[2] = (seq >> 8) & 0xff;
  buf[3] = 0x00;
  buf[4] = 0x00;

  bluetooth_tx_enqueue(buf, sizeof(buf));
}

static void ota_task(void *arg) {
  ota_packet_t pkt;

  while (1) {
    if (xQueueReceive(ota_queue, &pkt, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (pkt.cmd == OTA_CMD_START) {
      ota_reset();
      ota_partition = esp_ota_get_next_update_partition(NULL);
      esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
      ota_active = true;
      ota_send_resp(OTA_ACK, pkt.seq);
      continue;
    }

    if (!ota_active) {
      ota_send_resp(OTA_NACK, pkt.seq);
      continue;
    }

    if (pkt.cmd == OTA_CMD_DATA) {
      if (pkt.seq != expected_seq) {
        ota_send_resp(OTA_NACK, pkt.seq);
        continue;
      }

      esp_ota_write(ota_handle, pkt.data, pkt.len);
      expected_seq++;
      ota_send_resp(OTA_ACK, pkt.seq);
      continue;
    }

    if (pkt.cmd == OTA_CMD_END) {
      esp_ota_end(ota_handle);
      esp_ota_set_boot_partition(ota_partition);
      ota_send_resp(OTA_ACK, pkt.seq);
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_restart();
    }
  }
}

void ota_manager_init(void) {
  ota_queue = xQueueCreate(8, sizeof(ota_packet_t));
  ota_reset();

  xTaskCreatePinnedToCore(
    ota_task,
    "ota_task",
    8192,
    NULL,
    5,
    NULL,
    0
  );
}

void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  if (!data || len < 5) {
    return;
  }

  ota_packet_t pkt = {0};
  pkt.cmd = data[0];
  pkt.seq = data[1] | (data[2] << 8);
  pkt.len = data[3] | (data[4] << 8);

  if (pkt.len > 0 && len >= 5 + pkt.len) {
    memcpy(pkt.data, &data[5], pkt.len);
  }

  xQueueSend(ota_queue, &pkt, 0);
}

