#pragma once

#include <stdbool.h>

typedef enum {
  SYS_STATE_INIT,
  SYS_STATE_RUNNING,
  SYS_STATE_OTA
} system_state_t;

typedef void (*system_state_cb_t)(system_state_t state);

void system_state_init(void);
void system_state_set(system_state_t state);
system_state_t system_state_get(void);   // ✅ AÑADIR ESTA LÍNEA
bool system_is_running(void);
void system_register_state_cb(system_state_cb_t cb);

