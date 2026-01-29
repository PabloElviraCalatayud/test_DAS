#include "offline_agg.h"
#include <string.h>
#include <limits.h>
#include "esp_log.h"
#include <inttypes.h>


static const char *TAG = "OFFLINE_AGG";

static offline_hour_t s_hour;

static void init_minmax(void) {
  for (int i = 0; i < 6; i++) {
    s_hour.imu_max[i] = INT16_MIN;
    s_hour.imu_min[i] = INT16_MAX;
    s_hour.imu_sum[i] = 0;
  }
  s_hour.imu_n = 0;

  s_hour.pulse_max = 0;
  s_hour.pulse_min = UINT16_MAX;
  s_hour.pulse_sum = 0;
  s_hour.pulse_n = 0;
}

void offline_agg_init(void) {
  memset(&s_hour, 0, sizeof(s_hour));
  init_minmax();
}

void offline_agg_reset(uint32_t ts_hour_ms) {
  memset(&s_hour, 0, sizeof(s_hour));
  s_hour.ts_hour_ms = ts_hour_ms;
  init_minmax();
}

void offline_agg_update_imu(const int16_t imu6[6]) {
  for (int i = 0; i < 6; i++) {
    int16_t v = imu6[i];
    if (v > s_hour.imu_max[i]) s_hour.imu_max[i] = v;
    if (v < s_hour.imu_min[i]) s_hour.imu_min[i] = v;
    s_hour.imu_sum[i] += (int32_t)v;
  }
  s_hour.imu_n++;
}

void offline_agg_update_pulse(uint16_t pulse) {
  if (pulse > s_hour.pulse_max) s_hour.pulse_max = pulse;
  if (pulse < s_hour.pulse_min) s_hour.pulse_min = pulse;
  s_hour.pulse_sum += pulse;
  s_hour.pulse_n++;
}

esp_err_t offline_agg_flush_now(uint32_t ts_hour_ms) {
  // si no hay muestras, opcionalmente no guardes
  if (s_hour.imu_n == 0 && s_hour.pulse_n == 0) {
    ESP_LOGW(TAG, "Flush skipped (no samples)");
    offline_agg_reset(ts_hour_ms);
    return ESP_OK;
  }

  // asegura timestamp coherente
  s_hour.ts_hour_ms = ts_hour_ms;

  esp_err_t err = offline_storage_append_hour(&s_hour);
  ESP_LOGI("OFF", "saved count=%u ts=%" PRIu32,
         (unsigned)offline_storage_count(),s_hour.ts_hour_ms);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Saved offline hour. total=%u", (unsigned)offline_storage_count());
  } else {
    ESP_LOGE(TAG, "Failed saving offline hour: %s", esp_err_to_name(err));
  }

  offline_agg_reset(ts_hour_ms);
  return err;
}
