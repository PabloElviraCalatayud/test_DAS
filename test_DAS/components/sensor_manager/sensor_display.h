#pragma once

#include <stdint.h>

void sensor_display_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz
);

void sensor_display_feed_pulse_raw(uint16_t pulse_raw);

uint16_t sensor_display_get_pulse(void);
void sensor_display_get_accel(int16_t *ax, int16_t *ay, int16_t *az);

