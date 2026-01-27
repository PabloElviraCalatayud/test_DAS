#include "pulse_sensor.h"
#include "adc.h"
#include "packet_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"

#include "offline_agg.h"


static adc_continuous_handle_t s_adc;
static TaskHandle_t s_task;
static bool s_running;

static void pulse_task(void *arg) {
  adc_channel_result_t res = { .channel = ADC_CHANNEL_0 };

  while (s_running) {
    int n = adc_driver_read_multi(s_adc, &res, 1);
    if (n > 0) {
      //ESP_LOGI("PULSE", "RAW=%u ts=%llu", res.average, esp_timer_get_time() / 1000ULL);
      offline_agg_update_pulse((uint16_t)res.average);
      packet_feed_pulse_raw(res.average, esp_timer_get_time() / 1000ULL);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }

  adc_driver_deinit(s_adc);
  vTaskDelete(NULL);
}

void pulse_sensor_start(TaskHandle_t *out_task) {
  if (s_task) {
    return;
  }

  adc_driver_init(&s_adc);
  s_running = true;

  xTaskCreatePinnedToCore(pulse_task, "pulse", 4096, NULL, 3, &s_task, 1);

  if (out_task) {
    *out_task = s_task;
  }
}

void pulse_sensor_stop(void) {
  s_running = false;
}

