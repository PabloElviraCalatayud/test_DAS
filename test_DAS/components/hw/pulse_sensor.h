#pragma once

#include "freertos/FreeRTOS.h"

void pulse_sensor_start(TaskHandle_t *out_task);
void pulse_sensor_stop(void);

