#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "system_state.h"
#include "bluetooth.h"
#include "sensor_manager.h"
#include "ota_manager.h"
#include "packet_manager.h"

/* --- ADAPTADOR packet_manager -> BLE --- */
static void packet_tx_ble_adapter(const uint8_t *data, uint16_t len) {
  bluetooth_tx_enqueue(data, len);
}

void app_main(void) {
  system_state_init();

  bluetooth_init();
  ota_manager_init();

  packet_manager_init(packet_tx_ble_adapter);

  sensor_manager_init();

  system_state_set(SYS_STATE_RUNNING);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

