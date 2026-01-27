#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
  uint32_t ts_hour_ms;      // timestamp inicio de hora

  int16_t  imu_max[6];      // ax..gz
  int16_t  imu_min[6];
  int32_t  imu_sum[6];      // si acumulas MUCHO, cambia a int64
  uint32_t imu_n;

  uint16_t pulse_max;
  uint16_t pulse_min;
  uint32_t pulse_sum;
  uint32_t pulse_n;
} offline_hour_t;

// Inicializa y escanea la partición para encontrar el final del log
esp_err_t offline_storage_init(void);

// Devuelve cuántos registros válidos se detectaron en el scan (cacheado)
uint32_t offline_storage_count(void);

// Añade un registro (append). Devuelve ESP_ERR_NO_MEM si no cabe.
esp_err_t offline_storage_append_hour(const offline_hour_t *rec);

// Iterador sencillo de lectura (para cuando quieras enviar a BLE/app)
typedef struct {
  uint32_t offset;   // offset relativo dentro de la partición
  uint32_t index;    // índice de registro (0..count-1)
} offline_iter_t;

void offline_iter_begin(offline_iter_t *it);

// Lee el siguiente registro. Devuelve:
// - ESP_OK si devuelve uno
// - ESP_ERR_NOT_FOUND si no hay más
// - ESP_ERR_INVALID_CRC si encuentra corrupción (o algo raro)
esp_err_t offline_iter_next(offline_iter_t *it, offline_hour_t *out);

// Borra toda la partición (reset log)
esp_err_t offline_storage_erase_all(void);

#ifdef __cplusplus
}
#endif
