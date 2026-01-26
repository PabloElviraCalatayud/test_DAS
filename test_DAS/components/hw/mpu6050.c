#include "mpu6050.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "packet_manager.h"
#include "system_state.h"

#define TAG "MPU"

#define MPU_ADDR        0x68
#define REG_PWR_MGMT_1  0x6B
#define REG_ACCEL_XOUT  0x3B
#define REG_WHO_AM_I    0x75

static TaskHandle_t s_task;
static i2c_port_t s_port = I2C_NUM_1;
static volatile bool s_running;
static SemaphoreHandle_t s_stop_sem;

static esp_err_t mpu_write(uint8_t reg, uint8_t val) {
  uint8_t buf[2] = { reg, val };
  return i2c_master_write_to_device(
    s_port, MPU_ADDR, buf, 2, pdMS_TO_TICKS(50)
  );
}

static esp_err_t mpu_read(uint8_t reg, uint8_t *data, size_t len) {
  return i2c_master_write_read_device(
    s_port,
    MPU_ADDR,
    &reg,
    1,
    data,
    len,
    pdMS_TO_TICKS(50)
  );
}

static void mpu6050_task(void *arg) {
  uint8_t raw[14];

  while (s_running) {

    if (!system_is_running()) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    if (mpu_read(REG_ACCEL_XOUT, raw, sizeof(raw)) == ESP_OK) {
      int16_t ax = (raw[0] << 8) | raw[1];
      int16_t ay = (raw[2] << 8) | raw[3];
      int16_t az = (raw[4] << 8) | raw[5];
      int16_t gx = (raw[8] << 8) | raw[9];
      int16_t gy = (raw[10] << 8) | raw[11];
      int16_t gz = (raw[12] << 8) | raw[13];

      packet_feed_imu_raw(
        ax, ay, az,
        gx, gy, gz,
        esp_timer_get_time() / 1000ULL
      );
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }

  ESP_LOGI(TAG, "Task loop exited");
  
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

esp_err_t mpu6050_start(
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t freq,
  uint32_t stack,
  UBaseType_t prio,
  TaskHandle_t *out
) {
  if (s_task) return ESP_OK;

  if (!s_stop_sem) {
    s_stop_sem = xSemaphoreCreateBinary();
  }

  s_running = true;

  i2c_config_t cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = sda,
    .scl_io_num = scl,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = freq
  };

  ESP_ERROR_CHECK(i2c_param_config(s_port, &cfg));
  ESP_ERROR_CHECK(i2c_driver_install(s_port, cfg.mode, 0, 0, 0));

  uint8_t who = 0;
  if (mpu_read(REG_WHO_AM_I, &who, 1) != ESP_OK || who != 0x68) {
    ESP_LOGE(TAG, "WHO_AM_I failed (0x%02X)", who);
    i2c_driver_delete(s_port);
    s_running = false;
    return ESP_FAIL;
  }

  mpu_write(REG_PWR_MGMT_1, 0x00);
  vTaskDelay(pdMS_TO_TICKS(10));

  xTaskCreatePinnedToCore(
    mpu6050_task,
    "mpu6050",
    stack,
    NULL,
    prio,
    &s_task,
    1
  );

  if (out) *out = s_task;
  
  ESP_LOGI(TAG, "MPU6050 started");
  
  return ESP_OK;
}

void mpu6050_stop(void) {
  if (!s_task) return;
  
  ESP_LOGI(TAG, "Requesting stop...");
  s_running = false;
  
  if (s_stop_sem && xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(500)) == pdTRUE) {
    ESP_LOGI(TAG, "Task loop exited, cleaning up I2C...");
    
    // Limpieza desde sensor_manager_task
    i2c_driver_delete(s_port);
    s_task = NULL;
    
    ESP_LOGI(TAG, "I2C cleanup complete");
  } else {
    ESP_LOGE(TAG, "TIMEOUT waiting for task!");
  }
}
