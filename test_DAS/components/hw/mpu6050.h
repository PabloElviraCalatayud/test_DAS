#pragma once

#include "esp_err.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

esp_err_t mpu6050_start(
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t clk_hz,
  uint32_t stack_size,
  UBaseType_t task_prio,
  TaskHandle_t *out_task
);

void mpu6050_stop(void);

