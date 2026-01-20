#pragma once

#include <stdbool.h>

typedef enum {
  SYS_STATE_IDLE = 0,
  SYS_STATE_RUNNING,
  SYS_STATE_OTA,
  SYS_STATE_ERROR
} system_state_t;

typedef void (*system_state_cb_t)(system_state_t state);

void system_state_init(void);

system_state_t system_state_get(void);
bool system_state_set(system_state_t new_state);

bool system_is_running(void);
bool system_is_ota(void);

void system_register_state_cb(system_state_cb_t cb);

