#include "sensor_manager.h"
#include "system_state.h"

#include "mpu6050.h"
#include "pulse_sensor.h"

#include "esp_log.h"

#include "freertos/task.h"
#include "esp_timer.h"
#include "offline_agg.h"
#include "offline_storage.h"

static const char *TAG = "SENSOR_MGR";

static bool s_initialized;
static bool s_running;

static TaskHandle_t s_mpu_task;
static TaskHandle_t s_pulse_task;
static TaskHandle_t s_offline_task;


static void system_state_cb(system_state_t state) {
  if (state == SYS_STATE_OTA) {
    sensor_manager_stop();
  } else if (state == SYS_STATE_RUNNING) {
    sensor_manager_start();
  }
}

static uint32_t hour_start_ms(uint32_t now_ms) {
  const uint32_t hour_ms = 3600u * 1000u;
  return (now_ms / hour_ms) * hour_ms;
}







static void offline_task(void *arg) {
  const uint32_t hour_ms = 3600u * 1000u;

  uint32_t now = (uint32_t)(esp_timer_get_time() / 1000ULL);
  uint32_t cur_hour = hour_start_ms(now);

  offline_agg_init();
  offline_agg_reset(cur_hour);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));

    now = (uint32_t)(esp_timer_get_time() / 1000ULL);
    uint32_t h = hour_start_ms(now);
	
	static int ticks = 0;
	ticks++;

	
    if (h != cur_hour) {
      // se cerró la hora anterior
      offline_agg_flush_now(cur_hour);
      cur_hour = h;
    }
  }
}


esp_err_t sensor_manager_init(void) {
	

  ESP_LOGI(TAG, "Init sensor manager");
  offline_agg_init();

  if (s_initialized) {
    return ESP_OK;
  }

  system_register_state_cb(system_state_cb);

  s_initialized = true;
  return ESP_OK;
}

void sensor_manager_start(void) {
  if (s_running) return;

  ESP_LOGI(TAG, "Starting MPU6050");
  mpu6050_start(GPIO_NUM_21, GPIO_NUM_22, 100000, 4096, 3, &s_mpu_task);

  ESP_LOGI(TAG, "Starting pulse sensor");
  pulse_sensor_start(&s_pulse_task);

  s_running = true;
   if (s_offline_task == NULL) {
    xTaskCreate(offline_task, "offline_task", 4096, NULL, 3, &s_offline_task);
  }
}

void sensor_manager_stop(void) {
  if (!s_running) return;

  ESP_LOGW(TAG, "Stopping sensors");

  mpu6050_stop();
  pulse_sensor_stop();

  s_running = false;
  
  if (s_offline_task) {
    vTaskDelete(s_offline_task);
    s_offline_task = NULL;
  }

}

