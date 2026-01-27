#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "offline_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

void offline_agg_init(void);
void offline_agg_reset(uint32_t ts_hour_ms);

// Llamar cuando tengas una muestra IMU (ax..gz)
void offline_agg_update_imu(const int16_t imu6[6]);

// Llamar cuando tengas una muestra de pulso
void offline_agg_update_pulse(uint16_t pulse);

// Fuerza flush del registro actual (guarda y resetea)
esp_err_t offline_agg_flush_now(uint32_t ts_hour_ms);

#ifdef __cplusplus
}
#endif
