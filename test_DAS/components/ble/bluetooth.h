#pragma once

#include <stdint.h>

void bluetooth_init(void);

int bluetooth_notify(
  const uint8_t *data,
  uint16_t len
);

