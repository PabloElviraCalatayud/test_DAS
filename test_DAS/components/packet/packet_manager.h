#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "packet_format.h"

typedef void (*packet_tx_cb_t)(const uint8_t *data, uint16_t len);

esp_err_t packet_manager_init(packet_tx_cb_t tx_cb);

int packet_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz,
  uint32_t ts_ms
);

int packet_feed_pulse_raw(uint16_t pulse, uint32_t ts_ms);

