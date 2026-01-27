#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "bluetooth.h"
#include "packet_manager.h"
//#include "ota_manager.h"
#include "system_state.h"
#include "offline_storage.h"
#include "sensor_manager.h"

#include <inttypes.h>


static const char *TAG = "MAIN";

static void ble_tx_cb(const uint8_t *data, uint16_t len) {
  //ESP_LOGI(TAG, "BLE TX enqueue len=%u", len);
  bluetooth_tx_enqueue(data, len);
}

static void offline_dump_all(void)
{
    offline_iter_t it;
    offline_iter_begin(&it);

    offline_hour_t out;
    while (offline_iter_next(&it, &out) == ESP_OK) {
        ESP_LOGI("OFF_DUMP",
            "ts=%" PRIu32 " imu_n=%" PRIu32 " pulse_n=%" PRIu32
            " pulse[min=%" PRIu16 " max=%" PRIu16 " sum=%" PRIu32 "]"
            " imu_max0=%" PRId16 " imu_min0=%" PRId16 " imu_sum0=%" PRId32,
            out.ts_hour_ms, out.imu_n, out.pulse_n,
            out.pulse_min, out.pulse_max, out.pulse_sum,
            out.imu_max[0], out.imu_min[0], (int32_t)out.imu_sum[0]
        );
    }
}


void app_main(void) {
  ESP_LOGI(TAG, "System boot");

  system_state_init();
  ESP_LOGI(TAG, "System state initialized");
	
  ESP_ERROR_CHECK(offline_storage_init());
  ESP_LOGI(TAG, "Offline storage initialized, count=%u", (unsigned)offline_storage_count());
  offline_dump_all();
  
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
