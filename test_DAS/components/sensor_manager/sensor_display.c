#include "sensor_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static SemaphoreHandle_t s_mutex;

static uint16_t s_pulse_bpm;
static int16_t s_ax;
static int16_t s_ay;
static int16_t s_az;

static void ensure_mutex(void) {
  if (!s_mutex) {
    s_mutex = xSemaphoreCreateMutex();
  }
}

static uint16_t pulse_raw_to_bpm(uint16_t raw) {
  return raw;
}

static int16_t imu_raw_to_display(int16_t raw) {
  return raw;
}

void sensor_display_feed_imu_raw(
  int16_t ax, int16_t ay, int16_t az,
  int16_t gx, int16_t gy, int16_t gz
) {
  ensure_mutex();

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  s_ax = imu_raw_to_display(ax);
  s_ay = imu_raw_to_display(ay);
  s_az = imu_raw_to_display(az);

  xSemaphoreGive(s_mutex);
}

void sensor_display_feed_pulse_raw(uint16_t pulse_raw) {
  ensure_mutex();

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  s_pulse_bpm = pulse_raw_to_bpm(pulse_raw);

  xSemaphoreGive(s_mutex);
}

uint16_t sensor_display_get_pulse(void) {
  uint16_t v;

  ensure_mutex();

  xSemaphoreTake(s_mutex, portMAX_DELAY);
  v = s_pulse_bpm;
  xSemaphoreGive(s_mutex);

  return v;
}

void sensor_display_get_accel(int16_t *ax, int16_t *ay, int16_t *az) {
  ensure_mutex();

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  *ax = s_ax;
  *ay = s_ay;
  *az = s_az;

  xSemaphoreGive(s_mutex);
}

