#include "sensor_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <stdint.h>
#include <stdbool.h>

/* ===================== IMU ===================== */

#define ACC_LSB_PER_G     16384.0f
#define GYRO_LSB_PER_DPS  131.0f
#define GRAVITY_MS2       9.81f

/* ===================== PULSE TUNING (MERGE) ===================== */
/*
 * threshold = alpha * threshold + (1-alpha) * raw
 * - alpha alto = umbral más “lento”/estable (ej: 0.98)
 * - alpha bajo = umbral más reactivo (ej: 0.95)
 */
#ifndef PULSE_THRESH_ALPHA
#define PULSE_THRESH_ALPHA 0.98f
#endif

#ifndef PULSE_MARGIN
#define PULSE_MARGIN 300.0f   // tu: 250, suyo: 300
#endif

#ifndef MIN_PULSE_INTERVAL_US
#define MIN_PULSE_INTERVAL_US 300000u   // 0.3s => 200 bpm aprox
#endif

#ifndef MAX_PULSE_INTERVAL_US
#define MAX_PULSE_INTERVAL_US 1500000u  // 1.5s => 40 bpm aprox
#endif

#ifndef BPM_MIN
#define BPM_MIN 40.0f
#endif

#ifndef BPM_MAX
#define BPM_MAX 180.0f
#endif

#ifndef INTERVAL_AVG_COUNT
#define INTERVAL_AVG_COUNT 5
#endif

/* Suavizado opcional del BPM (tu estilo: EMA).
 * 0.0f = sin suavizado (actualiza directo)
 * 0.2f = parecido a tu 0.8/0.2
 */
#ifndef PULSE_BPM_SMOOTH_ALPHA
#define PULSE_BPM_SMOOTH_ALPHA 0.2f
#endif

/* Offset opcional del BPM (por si alguien metió un “-100” para calibrar UI)
 * Ej: -100 para restar 100, 0 para normal.
 */
#ifndef PULSE_BPM_OFFSET
#define PULSE_BPM_OFFSET 0
#endif

static SemaphoreHandle_t s_mutex;

static void ensure_mutex(void) {
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
  }
}

/* ===================== IMU STORAGE ===================== */

static int16_t s_ax_cms2;
static int16_t s_ay_cms2;
static int16_t s_az_cms2;

static int16_t s_gx_dps;
static int16_t s_gy_dps;
static int16_t s_gz_dps;

/* ===================== PULSE STORAGE ===================== */

static float    s_threshold = 2000.0f;
static bool     s_pulse_detected;
static uint64_t s_last_pulse_time_us;
static float    s_last_bpm;

static uint32_t s_intervals[INTERVAL_AVG_COUNT];
static uint8_t  s_interval_index;
static uint8_t  s_interval_count;

/* ===================== IMU FEED ===================== */

void sensor_display_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz
) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);

  float ax_ms2 = (ax / ACC_LSB_PER_G) * GRAVITY_MS2;
  float ay_ms2 = (ay / ACC_LSB_PER_G) * GRAVITY_MS2;
  float az_ms2 = (az / ACC_LSB_PER_G) * GRAVITY_MS2;

  s_ax_cms2 = (int16_t)(ax_ms2 * 100.0f);
  s_ay_cms2 = (int16_t)(ay_ms2 * 100.0f);
  s_az_cms2 = (int16_t)(az_ms2 * 100.0f);

  s_gx_dps = (int16_t)(gx / GYRO_LSB_PER_DPS);
  s_gy_dps = (int16_t)(gy / GYRO_LSB_PER_DPS);
  s_gz_dps = (int16_t)(gz / GYRO_LSB_PER_DPS);

  xSemaphoreGive(s_mutex);
}

/* ===================== PULSE FEED ===================== */

static float clampf(float v, float lo, float hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

void sensor_display_feed_pulse_raw(uint16_t raw, uint32_t ts_ms) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);

  uint64_t now_us = (uint64_t)ts_ms * 1000ULL;

  // Umbral adaptativo (merge: configurable)
  s_threshold = (PULSE_THRESH_ALPHA * s_threshold) + ((1.0f - PULSE_THRESH_ALPHA) * (float)raw);

  // Disparo por flanco (raw > threshold + margen)
  if (!s_pulse_detected && ((float)raw > (s_threshold + PULSE_MARGIN))) {

    if (s_last_pulse_time_us > 0) {
      uint64_t interval_us_64 = now_us - s_last_pulse_time_us;

      // Filtrado por rango de intervalos válido
      if (interval_us_64 >= (uint64_t)MIN_PULSE_INTERVAL_US &&
          interval_us_64 <= (uint64_t)MAX_PULSE_INTERVAL_US) {

        uint32_t interval_us = (uint32_t)interval_us_64;

        // Ventana circular de intervalos (merge: suyo)
        s_intervals[s_interval_index] = interval_us;
        s_interval_index = (uint8_t)((s_interval_index + 1) % INTERVAL_AVG_COUNT);
        if (s_interval_count < INTERVAL_AVG_COUNT) {
          s_interval_count++;
        }

        // Promedio de intervalos
        uint64_t sum = 0;
        for (uint8_t i = 0; i < s_interval_count; i++) {
          sum += s_intervals[i];
        }

        float avg_interval_s = ((float)sum / (float)s_interval_count) / 1000000.0f;
        if (avg_interval_s > 0.0f) {
          float bpm = 60.0f / avg_interval_s;
          bpm = clampf(bpm, 0.0f, 1000.0f);

          // Rango BPM válido (merge: suyo)
          if (bpm >= BPM_MIN && bpm <= BPM_MAX) {
            // Suavizado opcional (merge: tuyo)
            if (PULSE_BPM_SMOOTH_ALPHA > 0.0f && s_last_bpm > 0.0f) {
              s_last_bpm = (1.0f - PULSE_BPM_SMOOTH_ALPHA) * s_last_bpm +
                           (PULSE_BPM_SMOOTH_ALPHA) * bpm;
            } else {
              s_last_bpm = bpm;
            }
          }
        }
      }
    }

    s_last_pulse_time_us = now_us;
    s_pulse_detected = true;
  }

  // Reset de detección cuando baja de umbral
  if (s_pulse_detected && ((float)raw < s_threshold)) {
    s_pulse_detected = false;
  }

  xSemaphoreGive(s_mutex);
}

/* ===================== GETTERS ===================== */

uint16_t sensor_display_get_pulse_bpm(void) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  float bpm_f = s_last_bpm;
  xSemaphoreGive(s_mutex);

  int bpm_i = (int)(bpm_f + 0.5f) + (int)PULSE_BPM_OFFSET;

  if (bpm_i < 0) {
    return 0;
  }
  if (bpm_i > 65535) {
    return 65535;
  }
  return (uint16_t)bpm_i;
}

void sensor_display_get_accel(
  int16_t *ax_cms2,
  int16_t *ay_cms2,
  int16_t *az_cms2
) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);

  *ax_cms2 = s_ax_cms2;
  *ay_cms2 = s_ay_cms2;
  *az_cms2 = s_az_cms2;

  xSemaphoreGive(s_mutex);
}

void sensor_display_get_gyro(
  int16_t *gx_dps,
  int16_t *gy_dps,
  int16_t *gz_dps
) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);

  *gx_dps = s_gx_dps;
  *gy_dps = s_gy_dps;
  *gz_dps = s_gz_dps;

  xSemaphoreGive(s_mutex);
}
55