#include "sensor_manager.h"
#include "system_state.h"

#include "mpu6050.h"
#include "pulse_sensor.h"

#include "esp_log.h"

static const char *TAG = "SENSOR_MGR";

static bool s_initialized;
static bool s_running;

static TaskHandle_t s_mpu_task;
static TaskHandle_t s_pulse_task;

static void system_state_cb(system_state_t state) {
  if (state == SYS_STATE_OTA) {
    sensor_manager_stop();
  } else if (state == SYS_STATE_RUNNING) {
    sensor_manager_start();
  }
}

esp_err_t sensor_manager_init(void) {
  ESP_LOGI(TAG, "Init sensor manager");

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
}

void sensor_manager_stop(void) {
  if (!s_running) return;

  ESP_LOGW(TAG, "Stopping sensors");

  mpu6050_stop();
  pulse_sensor_stop();

  s_running = false;
}
