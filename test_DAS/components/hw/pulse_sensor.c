#include "pulse_sensor.h"
#include "adc.h"
#include "packet_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <math.h>

#define REPORT_PERIOD_MS 500

static adc_continuous_handle_t adc_handle;

static float s_last_bpm;
static uint16_t s_last_raw;

static float pulse_raw_to_bpm(uint16_t raw, int64_t now_us) {
  static float threshold = 2000.0f;
  static bool pulse_detected = false;
  static int64_t last_pulse_time = 0;

  threshold = 0.95f * threshold + 0.05f * raw;

  if (!pulse_detected && raw > threshold + 250) {
    pulse_detected = true;

    if (last_pulse_time > 0) {
      float interval_s = (now_us - last_pulse_time) / 1000000.0f;
      float bpm = 60.0f / interval_s;
      s_last_bpm = 0.8f * s_last_bpm + 0.2f * bpm;
    }

    last_pulse_time = now_us;
  }

  if (pulse_detected && raw < threshold) {
    pulse_detected = false;
  }

  return s_last_bpm;
}

static void pulse_sensor_task(void *arg) {
  adc_channel_result_t results[] = {
    {.channel = ADC_CHANNEL_0, .average = 0},
  };

  int64_t last_report = 0;

  while (1) {
    int samples = adc_driver_read_multi(adc_handle, results, 1);
    if (samples <= 0) {
      vTaskDelay(pdMS_TO_TICKS(5));
      continue;
    }

    uint16_t raw = results[0].average;
    int64_t now_us = esp_timer_get_time();
    uint32_t ts_ms = now_us / 1000ULL;

    s_last_raw = raw;
    pulse_raw_to_bpm(raw, now_us);

    packet_feed_pulse_raw(raw, ts_ms);

    if ((now_us - last_report) > REPORT_PERIOD_MS * 1000) {
      last_report = now_us;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void pulse_sensor_start(void) {
  adc_driver_init(&adc_handle);

  xTaskCreate(
    pulse_sensor_task,
    "pulse_sensor_task",
    4096,
    NULL,
    5,
    NULL
  );
}

float pulse_sensor_get_bpm(void) {
  return s_last_bpm;
}

uint16_t pulse_sensor_get_raw(void) {
  return s_last_raw;
}

