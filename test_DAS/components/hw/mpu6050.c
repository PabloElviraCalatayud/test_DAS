#include "mpu6050.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "ota_manager.h"

#define I2C_PORT I2C_NUM_0
#define MPU_ADDR 0x68

#define REG_PWR_MGMT_1     0x6B
#define REG_ACCEL_XOUT_H   0x3B

static QueueHandle_t s_queue;
static mpu6050_raw_t s_last_raw;
static bool s_has_sample;

static esp_err_t mpu_read(uint8_t reg, uint8_t *buf, size_t len) {
  return i2c_master_write_read_device(
    I2C_PORT,
    MPU_ADDR,
    &reg,
    1,
    buf,
    len,
    pdMS_TO_TICKS(50)
  );
}

static void mpu_task(void *arg) {
  uint8_t d[14];
  mpu6050_raw_t raw;

  while (1) {
    if (!ota_manager_is_active()) {
      if (mpu_read(REG_ACCEL_XOUT_H, d, 14) == ESP_OK) {
        raw.ax = (int16_t)((d[0] << 8) | d[1]);
        raw.ay = (int16_t)((d[2] << 8) | d[3]);
        raw.az = (int16_t)((d[4] << 8) | d[5]);
        raw.gx = (int16_t)((d[8] << 8) | d[9]);
        raw.gy = (int16_t)((d[10] << 8) | d[11]);
        raw.gz = (int16_t)((d[12] << 8) | d[13]);

        s_last_raw = raw;
        s_has_sample = true;

        xQueueOverwrite(s_queue, &raw);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

esp_err_t mpu6050_start(
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t clk_hz,
  uint32_t stack_size,
  UBaseType_t task_prio
) {
  i2c_config_t cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = sda,
    .scl_io_num = scl,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = clk_hz
  };

  ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

  uint8_t cmd[2] = {REG_PWR_MGMT_1, 0x00};
  ESP_ERROR_CHECK(i2c_master_write_to_device(
    I2C_PORT,
    MPU_ADDR,
    cmd,
    2,
    pdMS_TO_TICKS(100)
  ));

  s_queue = xQueueCreate(1, sizeof(mpu6050_raw_t));
  if (!s_queue) {
    return ESP_ERR_NO_MEM;
  }

  xTaskCreate(
    mpu_task,
    "mpu6050",
    stack_size,
    NULL,
    task_prio,
    NULL
  );

  return ESP_OK;
}

esp_err_t mpu6050_calibrate(size_t samples) {
  uint8_t d[14];

  for (size_t i = 0; i < samples; i++) {
    if (!ota_manager_is_active()) {
      mpu_read(REG_ACCEL_XOUT_H, d, 14);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  return ESP_OK;
}

esp_err_t mpu6050_get_raw(mpu6050_raw_t *out, TickType_t timeout) {
  if (!out) {
    return ESP_ERR_INVALID_ARG;
  }

  if (xQueueReceive(s_queue, out, timeout) != pdTRUE) {
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}

esp_err_t mpu6050_get_latest(mpu6050_data_t *out, TickType_t timeout) {
  if (!out) {
    return ESP_ERR_INVALID_ARG;
  }

  TickType_t start = xTaskGetTickCount();

  while (!s_has_sample) {
    if ((xTaskGetTickCount() - start) > timeout) {
      return ESP_ERR_TIMEOUT;
    }
    vTaskDelay(1);
  }

  out->accel_x = (float)s_last_raw.ax / 16384.0f * 9.80665f;
  out->accel_y = (float)s_last_raw.ay / 16384.0f * 9.80665f;
  out->accel_z = (float)s_last_raw.az / 16384.0f * 9.80665f;
  out->gyro_x = (float)s_last_raw.gx / 131.0f;
  out->gyro_y = (float)s_last_raw.gy / 131.0f;
  out->gyro_z = (float)s_last_raw.gz / 131.0f;

  return ESP_OK;
}

