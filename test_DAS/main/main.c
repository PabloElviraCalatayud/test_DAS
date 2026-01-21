#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "bluetooth.h"
#include "packet_manager.h"
//#include "ota_manager.h"
#include "system_state.h"
#include "sensor_manager.h"

static const char *TAG = "MAIN";

static void ble_tx_cb(const uint8_t *data, uint16_t len) {
  ESP_LOGI(TAG, "BLE TX enqueue len=%u", len);
  bluetooth_tx_enqueue(data, len);
}

void app_main(void) {
  ESP_LOGI(TAG, "System boot");

  system_state_init();
  ESP_LOGI(TAG, "System state initialized");

  bluetooth_init();
  ESP_LOGI(TAG, "Bluetooth initialized");

  // OTA DESACTIVADA PARA TEST DE SENSORES
  // ota_manager_init();
  ESP_LOGW(TAG, "OTA manager DISABLED");

  sensor_manager_init();
  ESP_LOGI(TAG, "Sensor manager initialized");

  packet_manager_init(ble_tx_cb);
  ESP_LOGI(TAG, "Packet manager initialized");

  system_state_set(SYS_STATE_RUNNING);
  ESP_LOGI(TAG, "System state set to RUNNING");

  while (1) {
    ESP_LOGD(TAG, "Main loop alive");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

