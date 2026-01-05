#include "packet_manager.h"
#include "ota_manager.h"

#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#define PACKET_QUEUE_LEN 4

typedef struct {
  packet_t pkt;
} packet_item_t;

static packet_t s_work_pkt;
static QueueHandle_t s_queue;
static packet_tx_cb_t s_tx_cb;

static void packet_reset(uint32_t ts_ms) {
  memset(&s_work_pkt, 0, sizeof(s_work_pkt));
  s_work_pkt.header.version = PACKET_VERSION;
  s_work_pkt.header.type = PKT_TYPE_DATA;
  s_work_pkt.header.ts_ms = ts_ms;
}

static void packet_flush(void) {
  if (ota_manager_is_active()) {
    return;
  }

  packet_item_t item;
  memcpy(&item.pkt, &s_work_pkt, sizeof(packet_t));
  xQueueSend(s_queue, &item, 0);
}

static void packet_task(void *arg) {
  packet_item_t item;

  while (1) {
    if (xQueueReceive(s_queue, &item, portMAX_DELAY)) {

      if (ota_manager_is_active()) {
        continue;
      }

      if (s_tx_cb) {
        s_tx_cb((const uint8_t *)&item.pkt, sizeof(packet_t));
      }
    }
  }
}

static void packet_flush_task(void *arg) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(20));

    uint32_t ts_ms = esp_timer_get_time() / 1000ULL;

    if (s_work_pkt.header.imu_count > 0 &&
        s_work_pkt.header.pulse_count > 0) {

      packet_flush();
      packet_reset(ts_ms);
    }
  }
}

esp_err_t packet_manager_init(packet_tx_cb_t tx_cb) {
  s_tx_cb = tx_cb;

  s_queue = xQueueCreate(PACKET_QUEUE_LEN, sizeof(packet_item_t));
  if (!s_queue) {
    return ESP_ERR_NO_MEM;
  }

  packet_reset(esp_timer_get_time() / 1000ULL);

  xTaskCreate(packet_task, "packet_task", 4096, NULL, 5, NULL);
  xTaskCreate(packet_flush_task, "packet_flush_task", 2048, NULL, 4, NULL);

  return ESP_OK;
}

int packet_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz,
  uint32_t ts_ms
) {
  if (s_work_pkt.header.imu_count < PACKET_IMU_MAX) {
    uint8_t i = s_work_pkt.header.imu_count;

    s_work_pkt.imu[i].ax = ax;
    s_work_pkt.imu[i].ay = ay;
    s_work_pkt.imu[i].az = az;
    s_work_pkt.imu[i].gx = gx;
    s_work_pkt.imu[i].gy = gy;
    s_work_pkt.imu[i].gz = gz;

    s_work_pkt.header.imu_count++;
  }

  return 0;
}

int packet_feed_pulse_raw(uint16_t pulse, uint32_t ts_ms) {
  if (s_work_pkt.header.pulse_count < PACKET_PULSE_MAX) {
    uint8_t i = s_work_pkt.header.pulse_count;
    s_work_pkt.pulse[i] = pulse;
    s_work_pkt.header.pulse_count++;
  }

  return 0;
}

