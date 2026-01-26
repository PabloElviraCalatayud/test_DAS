#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_log.h"
#include "bluetooth.h"
#include "system_state.h"

#define TAG "OTA_MGR"

#define OTA_CMD_START  0x01
#define OTA_CMD_DATA   0x02
#define OTA_CMD_END    0x03
#define OTA_ACK        0x80
#define OTA_NACK       0x81

typedef struct {
  uint8_t cmd;
  uint16_t seq;
  uint16_t len;
  uint8_t data[240];
} ota_packet_t;

static QueueHandle_t s_queue;
static esp_ota_handle_t s_ota;
static const esp_partition_t *s_part;
static uint16_t s_seq;
static bool s_active;

static void ota_reset(void) {
  s_active = false;
  s_seq = 0;
  s_part = NULL;
  s_ota = 0;
}

static void ota_send(uint8_t cmd, uint16_t seq) {
  uint8_t b[5];
  b[0] = cmd;
  b[1] = seq & 0xff;
  b[2] = seq >> 8;
  b[3] = 0;
  b[4] = 0;
  
  ESP_LOGI(TAG, "Sending response: cmd=0x%02X, seq=%d", cmd, seq);
  
  bool sent = bluetooth_tx_enqueue(b, sizeof(b));
  if (!sent) {
    ESP_LOGE(TAG, "Failed to enqueue response!");
  }
}

static void ota_task(void *arg) {
  ota_packet_t p;

  while (1) {
    xQueueReceive(s_queue, &p, portMAX_DELAY);

    ESP_LOGI(TAG, "Received OTA command: 0x%02X, seq=%d, len=%d", p.cmd, p.seq, p.len);

    if (p.cmd == OTA_CMD_START) {
      ESP_LOGI(TAG, "OTA START command received");
      
      // Cambiar estado a OTA (solo una vez aquí)
      if (system_state_get() != SYS_STATE_OTA) {
        ESP_LOGI(TAG, "Changing to OTA state...");
        system_state_set(SYS_STATE_OTA);
        // Esperar a que se detengan los sensores
        vTaskDelay(pdMS_TO_TICKS(500));
      }

      ota_reset();
      s_part = esp_ota_get_next_update_partition(NULL);

      if (!s_part) {
        ESP_LOGE(TAG, "No OTA partition found");
        ota_send(OTA_NACK, p.seq);
        continue;
      }

      ESP_LOGI(TAG, "Starting OTA to partition %s at 0x%lx", 
               s_part->label, s_part->address);

      esp_err_t err = esp_ota_begin(s_part, OTA_SIZE_UNKNOWN, &s_ota);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        ota_send(OTA_NACK, p.seq);
        continue;
      }

      s_active = true;
      ESP_LOGI(TAG, "OTA session started, ready for data");
      ota_send(OTA_ACK, p.seq);
      continue;
    }

    if (!s_active) {
      ESP_LOGW(TAG, "Received cmd 0x%02X but session not active", p.cmd);
      ota_send(OTA_NACK, p.seq);
      continue;
    }

    if (p.cmd == OTA_CMD_DATA) {
      if (p.seq != s_seq) {
        ESP_LOGW(TAG, "Sequence mismatch: expected %d, got %d", s_seq, p.seq);
        ota_send(OTA_NACK, p.seq);
        continue;
      }

      esp_err_t err = esp_ota_write(s_ota, p.data, p.len);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed at seq %d: %s", p.seq, esp_err_to_name(err));
        ota_send(OTA_NACK, p.seq);
        continue;
      }

      s_seq++;
      
      // Log cada 50 paquetes
      if (s_seq % 50 == 0) {
        ESP_LOGI(TAG, "OTA progress: %d packets (%d bytes)", s_seq, s_seq * p.len);
      }
      
      ota_send(OTA_ACK, p.seq);
      continue;
    }

    if (p.cmd == OTA_CMD_END) {
      ESP_LOGI(TAG, "OTA END command received, finalizing...");
      
      esp_err_t err = esp_ota_end(s_ota);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        ota_send(OTA_NACK, p.seq);
        continue;
      }
      
      err = esp_ota_set_boot_partition(s_part);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        ota_send(OTA_NACK, p.seq);
        continue;
      }
      
      ESP_LOGI(TAG, "OTA complete! Rebooting in 500ms...");
      ota_send(OTA_ACK, p.seq);
      vTaskDelay(pdMS_TO_TICKS(500));
      esp_restart();
    }
  }
}

void ota_manager_init(void) {
  s_queue = xQueueCreate(16, sizeof(ota_packet_t)); // Aumentado de 8 a 16
  ota_reset();

  xTaskCreatePinnedToCore(
    ota_task,
    "ota",
    8192,
    NULL,
    5,
    NULL,
    0
  );
  
  ESP_LOGI(TAG, "OTA manager initialized");
}

void ota_manager_handle_packet(const uint8_t *data, uint16_t len) {
  if (!data || len < 5) {
    ESP_LOGW(TAG, "Invalid packet: len=%d", len);
    return;
  }

  ota_packet_t p = {0};
  p.cmd = data[0];
  p.seq = data[1] | (data[2] << 8);
  p.len = data[3] | (data[4] << 8);

  ESP_LOGI(TAG, "handle_packet: cmd=0x%02X, seq=%d, len=%d, total_len=%d", p.cmd, p.seq, p.len, len);

  if (p.len > 0) {
    if (len < 5 + p.len) {
      ESP_LOGE(TAG, "Packet too short: expected %d, got %d", 5 + p.len, len);
      return;
    }
    memcpy(p.data, &data[5], p.len);
  }

  if (xQueueSend(s_queue, &p, pdMS_TO_TICKS(100)) != pdTRUE) {
    ESP_LOGE(TAG, "Failed to enqueue OTA packet");
  }
}
