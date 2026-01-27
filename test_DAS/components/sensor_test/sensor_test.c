#include "sensor_test.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "packet_manager.h"
#include "system_state.h"

#include "esp_log.h"
#include "esp_random.h"

#include <math.h>

#define TAG "SENSOR_TEST"

#define TICK_MS 120000

static TaskHandle_t s_task;
static bool s_running;

static uint64_t s_sim_time = 0;

static float s_bpm = 40.0f;
static float s_bpm_target = 190.0f;
static float s_bpm_step = 2.5f;

static float s_imu_amp = 1.0f;
static float s_imu_target = 15.0f;
static float s_imu_step = 0.5f;

static void update_bpm(void) {
  if (fabsf(s_bpm - s_bpm_target) < 1.0f) {
    if (s_bpm_target > 150.0f) {
      s_bpm_target = 50.0f;
    } else {
      s_bpm_target = 190.0f;
    }
    s_bpm_step = (s_bpm_target > s_bpm) ? 3.0f : -3.0f;
  }

  s_bpm += s_bpm_step;
}

static void update_imu_amp(void) {
  if (fabsf(s_imu_amp - s_imu_target) < 0.2f) {
    if (s_imu_target > 10.0f) {
      s_imu_target = 1.0f;
    } else {
      s_imu_target = 15.0f;
    }
    s_imu_step = (s_imu_target > s_imu_amp) ? 0.7f : -0.7f;
  }

  s_imu_amp += s_imu_step;
}

static uint16_t simulate_pulse_raw(float bpm) {
  static float phase = 0.0f;

  float freq = bpm / 60.0f;
  phase += 2.0f * M_PI * freq * 0.02f;

  if (phase > 2.0f * M_PI) {
    phase -= 2.0f * M_PI;
  }

  float wave = sinf(phase);
  if (wave < 0.0f) {
    wave = 0.0f;
  }

  return 800 + (uint16_t)(wave * 2200.0f);
}

static void sensor_test_task(void *arg) {
  while (s_running) {

    if (!system_is_running()) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    s_sim_time += TICK_MS;

    update_bpm();
    update_imu_amp();

    int16_t ax = (int16_t)(s_imu_amp * 1000.0f * sinf(s_sim_time * 0.01f));
    int16_t ay = (int16_t)(s_imu_amp * 1000.0f * cosf(s_sim_time * 0.013f));
    int16_t az = 16384 + (int16_t)(s_imu_amp * 800.0f * sinf(s_sim_time * 0.02f));

    int16_t gx = (esp_random() % 800) - 400;
    int16_t gy = (esp_random() % 800) - 400;
    int16_t gz = (esp_random() % 800) - 400;

    packet_feed_imu_raw(
      ax, ay, az,
      gx, gy, gz,
      s_sim_time
    );

    uint16_t pulse_raw = simulate_pulse_raw(s_bpm);

    packet_feed_pulse_raw(
      pulse_raw,
      s_sim_time
    );

    vTaskDelay(pdMS_TO_TICKS(100));
  }

  ESP_LOGI(TAG, "Simulation task stopped");
  vTaskDelete(NULL);
}

void sensor_test_start(void) {
  if (s_task) {
    ESP_LOGW(TAG, "Sensor test already running");
    return;
  }

  s_running = true;
  s_sim_time = 0;

  xTaskCreatePinnedToCore(
    sensor_test_task,
    "sensor_test",
    4096,
    NULL,
    3,
    &s_task,
    1
  );
}

void sensor_test_stop(void) {
  if (!s_task) {
    return;
  }

  s_running = false;
  s_task = NULL;
}

