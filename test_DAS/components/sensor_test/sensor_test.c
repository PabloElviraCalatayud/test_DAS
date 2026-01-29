#include "sensor_test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "packet_manager.h"

static const char *TAG = "SENSOR_TEST";

static void imu_task(void *arg) {
  int16_t v = 0;

  while (1) {
    uint32_t ts = esp_log_timestamp();

    packet_feed_imu_raw(
      v, v + 1, v + 2,
      v + 3, v + 4, v + 5,
      ts
    );

    v += 10;

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

static void pulse_task(void *arg) {
  uint16_t pulse = 500;

  while (1) {
    uint32_t ts = esp_log_timestamp();

    packet_feed_pulse_raw(pulse, ts);

    pulse += 3;
    if (pulse > 800) {
      pulse = 500;
    }

    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

void sensor_test_start(void) {
  xTaskCreate(
    imu_task,
    "imu_test",
    2048,
    NULL,
    6,
    NULL
  );

  xTaskCreate(
    pulse_task,
    "pulse_test",
    2048,
    NULL,
    5,
    NULL
  );

  ESP_LOGI(TAG, "Sensor test started");
}

