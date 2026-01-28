#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "system_state.h"
#include "bluetooth.h"
#include "sensor_manager.h"
#include "ota_manager.h"
#include "packet_manager.h"
#include "OLED.h"
#include "sensor_test.h"

/* =========================================================
 *  CONFIGURACIÓN DE MODO
 * ========================================================= */
#define SENSOR_SIMULATION_MODE  0

/*  1 = Simulación
 *  0 = Sensores reales
 */

/* --- ADAPTADOR packet_manager -> BLE --- */
static void packet_tx_ble_adapter(const uint8_t *data, uint16_t len) {
  bluetooth_tx_enqueue(data, len);
}

void app_main(void) {
  system_state_init();

  bluetooth_init();
  ota_manager_init();

  packet_manager_init(packet_tx_ble_adapter);

#if SENSOR_SIMULATION_MODE
  sensor_test_start();
#else
  sensor_manager_init();
#endif

  system_state_set(SYS_STATE_RUNNING);
  
  
  OLED_start();
  

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

