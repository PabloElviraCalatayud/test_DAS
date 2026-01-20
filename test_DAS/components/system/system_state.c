#include "system_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define MAX_STATE_CBS 4

static system_state_t s_state = SYS_STATE_IDLE;
static SemaphoreHandle_t s_mutex;

static system_state_cb_t s_cbs[MAX_STATE_CBS];
static int s_cb_count;

void system_state_init(void) {
  s_mutex = xSemaphoreCreateMutex();
}

system_state_t system_state_get(void) {
  system_state_t state;

  xSemaphoreTake(s_mutex, portMAX_DELAY);
  state = s_state;
  xSemaphoreGive(s_mutex);

  return state;
}

bool system_state_set(system_state_t new_state) {
  system_state_cb_t cbs[MAX_STATE_CBS];
  int cb_count = 0;
  bool ok = false;

  xSemaphoreTake(s_mutex, portMAX_DELAY);

  switch (s_state) {
    case SYS_STATE_IDLE:
      ok = (new_state == SYS_STATE_RUNNING || new_state == SYS_STATE_ERROR);
      break;
    case SYS_STATE_RUNNING:
      ok = (new_state == SYS_STATE_OTA || new_state == SYS_STATE_ERROR);
      break;
    case SYS_STATE_OTA:
      ok = (new_state == SYS_STATE_RUNNING || new_state == SYS_STATE_ERROR);
      break;
    case SYS_STATE_ERROR:
      ok = false;
      break;
  }

  if (ok && s_state != new_state) {
    s_state = new_state;

    cb_count = s_cb_count;
    for (int i = 0; i < cb_count; i++) {
      cbs[i] = s_cbs[i];
    }
  }

  xSemaphoreGive(s_mutex);

  for (int i = 0; i < cb_count; i++) {
    cbs[i](new_state);
  }

  return ok;
}

bool system_is_running(void) {
  return system_state_get() == SYS_STATE_RUNNING;
}

bool system_is_ota(void) {
  return system_state_get() == SYS_STATE_OTA;
}

void system_register_state_cb(system_state_cb_t cb) {
  if (s_cb_count < MAX_STATE_CBS) {
    s_cbs[s_cb_count++] = cb;
  }
}

