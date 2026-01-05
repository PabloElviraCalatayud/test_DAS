#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"

typedef struct {
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
} mpu6050_raw_t;

typedef struct {
  float accel_x;
  float accel_y;
  float accel_z;
  float gyro_x;
  float gyro_y;
  float gyro_z;
} mpu6050_data_t;

esp_err_t mpu6050_start(
  i2c_port_t i2c_port,
  gpio_num_t sda,
  gpio_num_t scl,
  uint32_t clk_hz,
  uint32_t period_ms,
  uint32_t stack_size,
  UBaseType_t task_prio
);

esp_err_t mpu6050_calibrate(size_t samples);
esp_err_t mpu6050_get_raw(mpu6050_raw_t *out, TickType_t timeout);
esp_err_t mpu6050_get_latest(mpu6050_data_t *out, TickType_t timeout);

