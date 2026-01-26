#include "pulse_sensor.h"
#include "system_state.h"

#include "adc.h"
#include "packet_manager.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_timer.h"
#include "esp_log.h"

#define TAG "PULSE"

static adc_continuous_handle_t s_adc;
static TaskHandle_t s_task;
static volatile bool s_running;
static SemaphoreHandle_t s_stop_sem;

static void pulse_task(void *arg) {
  adc_channel_result_t res = { .channel = ADC_CHANNEL_0 };

  while (s_running) {

    if (!system_is_running()) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    // Verificación adicional justo antes de leer
    if (!s_running) {
      break;
    }

    int n = adc_driver_read_multi(s_adc, &res, 1);
    if (n > 0) {
      packet_feed_pulse_raw(
        res.average,
        esp_timer_get_time() / 1000ULL
      );
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }

  ESP_LOGI(TAG, "Task loop exited");
  xSemaphoreGive(s_stop_sem);
  vTaskDelete(NULL);
}

void pulse_sensor_start(TaskHandle_t *out_task) {
  if (s_task) {
    ESP_LOGW(TAG, "Already started");
    return;
  }

  if (!s_stop_sem) {
    s_stop_sem = xSemaphoreCreateBinary();
  }

  esp_err_t ret = adc_driver_init(&s_adc);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to init ADC");
    return;
  }
  
  s_running = true;

  xTaskCreatePinnedToCore(
    pulse_task,
    "pulse",
    4096,
    NULL,
    3,
    &s_task,
    1
  );

  if (out_task) *out_task = s_task;
  
  ESP_LOGI(TAG, "Pulse sensor started");
}

void pulse_sensor_stop(void) {
  if (!s_task) {
    ESP_LOGW(TAG, "Not running");
    return;
  }
  
  ESP_LOGI(TAG, "Requesting stop...");
  
  // CRÍTICO: Primero detener el ADC para liberar adc_driver_read_multi()
  ESP_LOGI(TAG, "Stopping ADC first to unblock task...");
  if (s_adc) {
    adc_continuous_stop(s_adc);
  }
  
  // LUEGO señalizar parada
  s_running = false;
  
  // Esperar a que la tarea termine
  if (xSemaphoreTake(s_stop_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
    ESP_LOGI(TAG, "Task exited, completing ADC cleanup...");
    
    // Completar limpieza del ADC
    if (s_adc) {
      vTaskDelay(pdMS_TO_TICKS(50));
      adc_continuous_deinit(s_adc);
      s_adc = NULL;
    }
    s_task = NULL;
    
    ESP_LOGI(TAG, "Cleanup complete");
  } else {
    ESP_LOGE(TAG, "TIMEOUT waiting for task! Force cleanup...");
    
    // Limpieza de emergencia
    if (s_adc) {
      adc_continuous_deinit(s_adc);
      s_adc = NULL;
    }
    s_task = NULL;
  }
}
