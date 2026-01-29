#include "sensor_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define ACC_LSB_PER_G     16384.0f
#define GYRO_LSB_PER_DPS  131.0f
#define GRAVITY_MS2       9.81f

#define MIN_PULSE_INTERVAL_US 300000
#define MAX_PULSE_INTERVAL_US 1500000
#define BPM_MIN 40.0f
#define BPM_MAX 180.0f
#define INTERVAL_AVG_COUNT 5

static SemaphoreHandle_t s_mutex;

/* ===================== IMU ===================== */

static int16_t s_ax_cms2;
static int16_t s_ay_cms2;
static int16_t s_az_cms2;

static int16_t s_gx_dps;
static int16_t s_gy_dps;
static int16_t s_gz_dps;

/* ===================== PULSE ===================== */

static float s_threshold = 2000.0f;
static bool s_pulse_detected;
static uint32_t s_last_pulse_time_us;
static float s_last_bpm;

static float s_intervals[INTERVAL_AVG_COUNT];
static uint8_t s_interval_index;
static uint8_t s_interval_count;

static void ensure_mutex(void) {
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
  }
}

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

void sensor_display_feed_pulse_raw(uint16_t raw, uint32_t ts_ms) {
  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);

  uint32_t now_us = ts_ms * 1000;

  s_threshold = 0.98f * s_threshold + 0.02f * raw;

  if (!s_pulse_detected && raw > s_threshold + 300.0f) {
    if (s_last_pulse_time_us > 0) {
      uint32_t interval_us = now_us - s_last_pulse_time_us;

      if (interval_us >= MIN_PULSE_INTERVAL_US &&
          interval_us <= MAX_PULSE_INTERVAL_US) {

        s_intervals[s_interval_index] = (float)interval_us;
        s_interval_index = (s_interval_index + 1) % INTERVAL_AVG_COUNT;
        if (s_interval_count < INTERVAL_AVG_COUNT) {
          s_interval_count++;
        }

        float sum = 0.0f;
        for (uint8_t i = 0; i < s_interval_count; i++) {
          sum += s_intervals[i];
        }

        float avg_interval_s = (sum / s_interval_count) / 1000000.0f;
        float bpm = 60.0f / avg_interval_s;

        if (bpm >= BPM_MIN && bpm <= BPM_MAX) {
          s_last_bpm = bpm;
        }
      }
    }

    s_last_pulse_time_us = now_us;
    s_pulse_detected = true;
  }

  if (s_pulse_detected && raw < s_threshold) {
    s_pulse_detected = false;
  }

  xSemaphoreGive(s_mutex);
}

/* ===================== GETTERS ===================== */

uint16_t sensor_display_get_pulse_bpm(void) {
  uint16_t bpm;

  ensure_mutex();
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  bpm = (uint16_t)(s_last_bpm + 0.5f);
  xSemaphoreGive(s_mutex);
  if(bpm - 100 < 0){
    return 0;
  }
  return bpm - 100;
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

