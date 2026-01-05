#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "bluetooth.h"
#include "packet_manager.h"
#include "mpu6050.h"
#include "pulse_sensor.h"
#include "ota_manager.h"

static const char *TAG = "MAIN";

/* --------------------------------------------------
 * BLE TX queue
 * -------------------------------------------------- */

typedef struct {
  uint16_t len;
  uint8_t data[247];
} ble_packet_t;

static QueueHandle_t ble_tx_queue;

/* --------------------------------------------------
 * BLE TX task (ÚNICO punto donde se hace notify)
 * -------------------------------------------------- */

static void ble_tx_task(void *arg) {
  ble_packet_t pkt;

  while (1) {
    if (xQueueReceive(ble_tx_queue, &pkt, portMAX_DELAY)) {
      bluetooth_notify(pkt.data, pkt.len);
      vTaskDelay(pdMS_TO_TICKS(30));
    }
  }
}

/* --------------------------------------------------
 * Packet manager callback (NO hace BLE)
 * -------------------------------------------------- */

static void ble_tx_cb(const uint8_t *data, uint16_t len) {
  if (len != 88) {
    ESP_LOGW("BLE", "Invalid packet size: %d", len);
    return;
  }

  bluetooth_notify(data, len);
}

/* --------------------------------------------------
 * Sensor / packet feed task
 * -------------------------------------------------- */

static void log_task(void *arg) {
  mpu6050_raw_t imu_raw;
  mpu6050_data_t imu_conv;

  while (1) {
    uint32_t ts_ms = esp_timer_get_time() / 1000ULL;

    uint16_t pulse_raw = pulse_sensor_get_raw();
    packet_feed_pulse_raw(pulse_raw, ts_ms);

    if (mpu6050_get_raw(&imu_raw, pdMS_TO_TICKS(10)) == ESP_OK &&
        mpu6050_get_latest(&imu_conv, pdMS_TO_TICKS(10)) == ESP_OK) {

      packet_feed_imu_raw(
        imu_raw.ax,
        imu_raw.ay,
        imu_raw.az,
        imu_raw.gx,
        imu_raw.gy,
        imu_raw.gz,
        ts_ms
      );
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

/* --------------------------------------------------
 * app_main
 * -------------------------------------------------- */

void app_main(void) {
  ESP_LOGI(TAG, "Init OTA");
  ota_manager_init();

  ESP_LOGI(TAG, "Init Bluetooth");
  bluetooth_init();

  ESP_LOGI(TAG, "Init BLE TX queue");
  ble_tx_queue = xQueueCreate(8, sizeof(ble_packet_t));

  xTaskCreate(
    ble_tx_task,
    "ble_tx",
    4096,
    NULL,
    5,
    NULL
  );

  ESP_LOGI(TAG, "Init packet manager");
  packet_manager_init(ble_tx_cb);

  ESP_LOGI(TAG, "Start MPU6050");
  mpu6050_start(
    I2C_NUM_0,
    GPIO_NUM_21,
    GPIO_NUM_22,
    400000,
    20,
    4096,
    5
  );

  vTaskDelay(pdMS_TO_TICKS(500));
  mpu6050_calibrate(300);

  ESP_LOGI(TAG, "Start pulse sensor");
  pulse_sensor_start();

  xTaskCreate(
    log_task,
    "log_task",
    4096,
    NULL,
    4,
    NULL
  );
}

