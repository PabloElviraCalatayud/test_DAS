#pragma once

#include <stdint.h>

void pulse_sensor_start(void);

float pulse_sensor_get_bpm(void);
uint16_t pulse_sensor_get_raw(void);

