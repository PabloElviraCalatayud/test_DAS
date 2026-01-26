#include "system_state.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "SYS_STATE"
#define MAX_STATE_CBS 4

static system_state_t s_state;
static SemaphoreHandle_t s_mutex;
static system_state_cb_t s_cbs[MAX_STATE_CBS];
static int s_cb_count;

void system_state_init(void) {
  s_mutex = xSemaphoreCreateMutex();
  s_state = SYS_STATE_INIT;
  s_cb_count = 0;
}

void system_register_state_cb(system_state_cb_t cb) {
  if (s_cb_count < MAX_STATE_CBS) {
    s_cbs[s_cb_count++] = cb;
  }
}

void system_state_set(system_state_t state) {
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  
  // Solo notificar si el estado realmente cambió
  if (s_state == state) {
    ESP_LOGW(TAG, "State already %d, ignoring", state);
    xSemaphoreGive(s_mutex);
    return;
  }
  
  ESP_LOGI(TAG, "State change: %d -> %d", s_state, state);
  s_state = state;
  xSemaphoreGive(s_mutex);

  // Notificar callbacks
  for (int i = 0; i < s_cb_count; i++) {
    s_cbs[i](state);
  }
}

system_state_t system_state_get(void) {
  system_state_t s;
  xSemaphoreTake(s_mutex, portMAX_DELAY);
  s = s_state;
  xSemaphoreGive(s_mutex);
  return s;
}

bool system_is_running(void) {
  return system_state_get() == SYS_STATE_RUNNING;
}
