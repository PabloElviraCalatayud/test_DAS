#pragma once

#include <stdint.h>

void sensor_display_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz
);

void sensor_display_feed_pulse_raw(uint16_t raw, uint32_t ts_ms);

uint16_t sensor_display_get_pulse_bpm(void);

void sensor_display_get_accel(
  int16_t *ax_cms2,
  int16_t *ay_cms2,
  int16_t *az_cms2
);

void sensor_display_get_gyro(
  int16_t *gx_dps,
  int16_t *gy_dps,
  int16_t *gz_dps
);

