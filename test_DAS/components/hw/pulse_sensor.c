#include "pulse_sensor.h"
#include "adc.h"
#include "packet_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"

static adc_continuous_handle_t s_adc;
static TaskHandle_t s_task = NULL;
static bool s_running = false;

static void pulse_task(void *arg) {
  adc_channel_result_t results[] = {
    { .channel = ADC_CHANNEL_0, .average = 0 },
  };

  while (s_running) {
    int samples = adc_driver_read_multi(s_adc, results, 1);
    if (samples > 0) {
      uint32_t ts = esp_timer_get_time() / 1000ULL;
      packet_feed_pulse_raw(results[0].average, ts);
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }

  adc_driver_deinit(s_adc);

  s_task = NULL;
  vTaskDelete(NULL);
}

void pulse_sensor_start(TaskHandle_t *out_task) {
  if (s_task) {
    if (out_task) {
      *out_task = s_task;
    }
    return;
  }

  adc_driver_init(&s_adc);

  s_running = true;

  xTaskCreate(
    pulse_task,
    "pulse_sensor",
    4096,
    NULL,
    5,
    &s_task
  );

  if (out_task) {
    *out_task = s_task;
  }
}

void pulse_sensor_stop(void) {
  if (!s_task) {
    return;
  }

  s_running = false;
}

