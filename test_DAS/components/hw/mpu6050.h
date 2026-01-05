#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

typedef struct {
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
} mpu6050_raw_t;

typedef struct {
  float accel_x, accel_y, accel_z;
  float gyro_x, gyro_y, gyro_z;
} mpu6050_data_t;

esp_err_t mpu6050_start(
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t clk_hz,
  uint32_t stack_size,
  UBaseType_t task_prio
);

esp_err_t mpu6050_calibrate(size_t samples);
esp_err_t mpu6050_get_raw(mpu6050_raw_t *out, TickType_t timeout);
esp_err_t mpu6050_get_latest(mpu6050_data_t *out, TickType_t timeout);

