#include "sensor_manager.h"

#include "system_state.h"
#include "mpu6050.h"
#include "pulse_sensor.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "offline_agg.h"
#include "offline_storage.h"

#define TAG "SENSOR_MGR"

typedef enum {
  SENSOR_CMD_START,
  SENSOR_CMD_STOP
} sensor_cmd_t;

static QueueHandle_t s_cmd_queue;
static TaskHandle_t s_mgr_task;
static TaskHandle_t s_offline_task;
static bool s_running;

/* ---------- Offline aggregation task ---------- */

static uint32_t hour_start_ms(uint32_t now_ms) {
  const uint32_t hour_ms = 3600u * 1000u;
  return (now_ms / hour_ms) * hour_ms;
}

static void offline_task(void *arg) {
  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
  uint32_t cur_hour = hour_start_ms(now);

  // Asegura estado consistente al arrancar
  offline_agg_reset(cur_hour);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));

    now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    uint32_t h = hour_start_ms(now);

    if (h != cur_hour) {
      // Se cerró la hora anterior
      offline_agg_flush_now(cur_hour);
      cur_hour = h;
      offline_agg_reset(cur_hour);
    }
  }
}

static void start_offline_task_if_needed(void) {
  if (!s_offline_task) {
    xTaskCreate(
      offline_task,
      "offline_task",
      4096,
      NULL,
      3,
      &s_offline_task
    );
  }
}

static void stop_offline_task_if_running(void) {
  if (s_offline_task) {
    // Flush final para no perder datos del tramo actual
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    uint32_t cur_hour = hour_start_ms(now);
    offline_agg_flush_now(cur_hour);

    vTaskDelete(s_offline_task);
    s_offline_task = NULL;
  }
}

/* ---------- Sensor manager task (serializa START/STOP) ---------- */

static void sensor_manager_task(void *arg) {
  sensor_cmd_t cmd;

  while (1) {
    if (xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
      continue;
    }

    if (cmd == SENSOR_CMD_STOP) {
      if (!s_running) continue;

      ESP_LOGW(TAG, "Stopping sensors");

   
      stop_offline_task_if_running();


      mpu6050_stop();
      vTaskDelay(pdMS_TO_TICKS(150));

      pulse_sensor_stop();
      vTaskDelay(pdMS_TO_TICKS(150));

      s_running = false;
      ESP_LOGI(TAG, "All sensors stopped");
    }

    if (cmd == SENSOR_CMD_START) {
      if (s_running) continue;

      ESP_LOGI(TAG, "Starting MPU6050");
      mpu6050_start(GPIO_NUM_21, GPIO_NUM_22, 100000, 4096, 3, NULL);

      vTaskDelay(pdMS_TO_TICKS(50));

      ESP_LOGI(TAG, "Starting pulse sensor");
      pulse_sensor_start(NULL);

      // Arrancar offline aggregation después de sensores
      start_offline_task_if_needed();

      s_running = true;
      ESP_LOGI(TAG, "All sensors started");
    }
  }
}

static void system_state_cb(system_state_t state) {
  sensor_cmd_t cmd;

  if (state == SYS_STATE_OTA) {
    ESP_LOGW(TAG, "OTA mode - stopping sensors");
    cmd = SENSOR_CMD_STOP;
    xQueueSend(s_cmd_queue, &cmd, 0);
  }

  if (state == SYS_STATE_RUNNING) {
    ESP_LOGI(TAG, "Running mode - starting sensors");
    cmd = SENSOR_CMD_START;
    xQueueSend(s_cmd_queue, &cmd, 0);
  }
}

esp_err_t sensor_manager_init(void) {
  s_running = false;

  // Init offline aggregation module una vez
  offline_agg_init();

  s_cmd_queue = xQueueCreate(4, sizeof(sensor_cmd_t));
  if (!s_cmd_queue) {
    return ESP_ERR_NO_MEM;
  }

  xTaskCreatePinnedToCore(
    sensor_manager_task,
    "sensor_mgr",
    4096,
    NULL,
    4,
    &s_mgr_task,
    1
  );

  system_register_state_cb(system_state_cb);

  ESP_LOGI(TAG, "Sensor manager initialized");
  return ESP_OK;
}
