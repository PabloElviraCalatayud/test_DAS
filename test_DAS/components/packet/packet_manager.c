#include "packet_manager.h"
#include "system_state.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_timer.h"

#define PACKET_QUEUE_LEN 4

typedef struct {
  packet_t pkt;
} packet_item_t;

static packet_t s_work_pkt;
static QueueHandle_t s_queue;
static SemaphoreHandle_t s_mutex;
static packet_tx_cb_t s_tx_cb;

static void packet_reset(uint32_t ts_ms) {
  memset(&s_work_pkt, 0, sizeof(s_work_pkt));
  s_work_pkt.header.version = PACKET_VERSION;
  s_work_pkt.header.type = PKT_TYPE_DATA;
  s_work_pkt.header.ts_ms = ts_ms;
}

static void packet_flush(void) {
  packet_item_t item;
  memcpy(&item.pkt, &s_work_pkt, sizeof(packet_t));
  xQueueSend(s_queue, &item, 0);
}

static void packet_task(void *arg) {
  packet_item_t item;

  while (1) {
    if (xQueueReceive(s_queue, &item, portMAX_DELAY)) {
      if (system_is_running() && s_tx_cb) {
        s_tx_cb((uint8_t *)&item.pkt, sizeof(packet_t));
      }
    }
    vTaskDelay(1);
  }
}

static void packet_flush_task(void *arg) {
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(20));

    if (!system_is_running()) continue;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    bool has_data =
      s_work_pkt.header.imu_count ||
      s_work_pkt.header.pulse_count;

    if (has_data) {
      packet_flush();
      packet_reset(esp_timer_get_time() / 1000ULL);
    }

    xSemaphoreGive(s_mutex);
  }
}

esp_err_t packet_manager_init(packet_tx_cb_t tx_cb) {
  s_tx_cb = tx_cb;
  s_queue = xQueueCreate(PACKET_QUEUE_LEN, sizeof(packet_item_t));
  s_mutex = xSemaphoreCreateMutex();

  packet_reset(esp_timer_get_time() / 1000ULL);

  xTaskCreate(packet_task, "pkt_tx", 4096, NULL, 5, NULL);
  xTaskCreate(packet_flush_task, "pkt_flush", 2048, NULL, 4, NULL);

  return ESP_OK;
}

int packet_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz,
  uint32_t ts_ms
) {
  if (!system_is_running()) return -1;

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  if (s_work_pkt.header.imu_count < PACKET_IMU_MAX) {
    uint8_t i = s_work_pkt.header.imu_count++;
    s_work_pkt.imu[i] = (packet_imu_raw_t){ ax, ay, az, gx, gy, gz };
  }

  xSemaphoreGive(s_mutex);
  return 0;
}

int packet_feed_pulse_raw(uint16_t pulse, uint32_t ts_ms) {
  if (!system_is_running()) return -1;

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  if (s_work_pkt.header.pulse_count < PACKET_PULSE_MAX) {
    s_work_pkt.pulse[s_work_pkt.header.pulse_count++] = pulse;
  }

  xSemaphoreGive(s_mutex);
  return 0;
}

