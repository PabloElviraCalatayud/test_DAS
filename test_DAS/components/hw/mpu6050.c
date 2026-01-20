#include "mpu6050.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"

static TaskHandle_t s_task = NULL;
static i2c_port_t s_port;
static uint32_t s_period_ms;
static bool s_running = false;

static void mpu6050_task(void *arg) {
  while (s_running) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd) {
      i2c_master_start(cmd);
      i2c_master_stop(cmd);
      i2c_master_cmd_begin(s_port, cmd, pdMS_TO_TICKS(50));
      i2c_cmd_link_delete(cmd);
    }

    vTaskDelay(pdMS_TO_TICKS(s_period_ms));
  }

  i2c_driver_delete(s_port);

  s_task = NULL;
  vTaskDelete(NULL);
}

esp_err_t mpu6050_start(
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t freq_hz,
  uint32_t stack_size,
  UBaseType_t priority,
  TaskHandle_t *out_task
) {
  if (s_task) {
    if (out_task) {
      *out_task = s_task;
    }
    return ESP_OK;
  }

  s_port = I2C_NUM_0;
  s_period_ms = 20;
  s_running = true;

  i2c_config_t cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = sda,
    .scl_io_num = scl,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = freq_hz
  };

  ESP_ERROR_CHECK(i2c_param_config(s_port, &cfg));
  ESP_ERROR_CHECK(i2c_driver_install(s_port, cfg.mode, 0, 0, 0));

  xTaskCreate(
    mpu6050_task,
    "mpu6050",
    stack_size,
    NULL,
    priority,
    &s_task
  );

  if (out_task) {
    *out_task = s_task;
  }

  return ESP_OK;
}

void mpu6050_stop(void) {
  if (!s_task) {
    return;
  }

  s_running = false;
}

