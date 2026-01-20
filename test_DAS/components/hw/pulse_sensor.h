#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"

void pulse_sensor_start(TaskHandle_t *out_task);
void pulse_sensor_stop(void);

uint16_t pulse_sensor_get_raw(void);

