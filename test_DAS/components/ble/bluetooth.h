#pragma once

#include <stdint.h>
#include <stdbool.h>

void bluetooth_init(void);
int bluetooth_notify(const uint8_t *data, uint16_t len);
bool bluetooth_tx_enqueue(const uint8_t *data, uint16_t len);

