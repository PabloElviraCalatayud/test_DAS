#pragma once

#include <stdint.h>
#include <stdbool.h>

void ota_manager_init(void);

void ota_manager_handle_packet(
  const uint8_t *data,
  uint16_t len
);

bool ota_manager_is_active(void);

