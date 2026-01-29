#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include <inttypes.h>

#include "system_state.h"
#include "bluetooth.h"
#include "packet_manager.h"
#include "sensor_manager.h"
#include "offline_storage.h"

#include "ota_manager.h"
#include "OLED.h"
#include "sensor_test.h"

/* =========================================================
 *  CONFIGURACIÓN DE MODO
 * ========================================================= */
#define SENSOR_SIMULATION_MODE  0   // 1 = simulación, 0 = sensores reales
#define OTA_ENABLED             1   // 1 = habilitar OTA, 0 = deshabilitar OTA

static const char *TAG = "MAIN";

/* --- Adaptador packet_manager -> BLE --- */
static void packet_tx_ble_adapter(const uint8_t *data, uint16_t len) {
  bluetooth_tx_enqueue(data, len);
}

/* --- Debug: volcar todo el offline storage --- */
static void offline_dump_all(void) {
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
  ESP_LOGI(TAG, "Offline storage initialized, count=%u",
           (unsigned)offline_storage_count());
  offline_dump_all();

  bluetooth_init();
  ESP_LOGI(TAG, "Bluetooth initialized");

#if OTA_ENABLED
  ota_manager_init();
  ESP_LOGI(TAG, "OTA manager initialized");
#else
  ESP_LOGW(TAG, "OTA manager DISABLED");
#endif


  packet_manager_init(packet_tx_ble_adapter);
  ESP_LOGI(TAG, "Packet manager initialized");

#if SENSOR_SIMULATION_MODE
  sensor_test_start();
  ESP_LOGW(TAG, "Sensor simulation mode enabled");
#else
  sensor_manager_init();
  ESP_LOGI(TAG, "Sensor manager initialized");
#endif

  system_state_set(SYS_STATE_RUNNING);
  ESP_LOGI(TAG, "System state set to RUNNING");

  OLED_start();

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
