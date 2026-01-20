#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bluetooth.h"
#include "packet_manager.h"
#include "ota_manager.h"
#include "system_state.h"
#include "sensor_manager.h"

static void ble_tx_cb(const uint8_t *data, uint16_t len) {
  if (!system_is_running()) {
    return;
  }

  bluetooth_tx_enqueue(data, len);
}


void app_main(void) {
  system_state_init();

  bluetooth_init();
  ota_manager_init();
  //sensor_manager_init();
  //packet_manager_init(ble_tx_cb);

  system_state_set(SYS_STATE_RUNNING);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

