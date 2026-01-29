#include "adc.h"
#include "esp_log.h"
#include <string.h>

#define TAG "ADC_DRV"
#define FRAME_SIZE 1024

// 0 = average, 1 = peak
#define ADC_USE_PEAK 0

static const adc_channel_t adc_channels[] = {
  ADC_CHANNEL_0,
};

esp_err_t adc_driver_init(adc_continuous_handle_t *out_handle) {
  adc_continuous_handle_cfg_t handle_cfg = {
    .max_store_buf_size = 1024,
    .conv_frame_size = 256,
  };

  adc_continuous_handle_t handle = NULL;
  esp_err_t ret = adc_continuous_new_handle(&handle_cfg, &handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "adc_continuous_new_handle failed: %s", esp_err_to_name(ret));
    return ret;
  }

  adc_digi_pattern_config_t pattern[1];
  pattern[0].atten = ADC_ATTEN_DB_12;
  pattern[0].channel = adc_channels[0];
  pattern[0].unit = ADC_UNIT_1;
  pattern[0].bit_width = ADC_BITWIDTH_12;

  adc_continuous_config_t cfg = {
    .sample_freq_hz = 20000,
    .conv_mode = ADC_CONV_SINGLE_UNIT_1,
    .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
    .pattern_num = 1,
    .adc_pattern = pattern,
  };

  ret = adc_continuous_config(handle, &cfg);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "adc_continuous_config failed: %s", esp_err_to_name(ret));
    adc_continuous_deinit(handle);
    return ret;
  }

  ret = adc_continuous_start(handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "adc_continuous_start failed: %s", esp_err_to_name(ret));
    adc_continuous_deinit(handle);
    return ret;
  }

  *out_handle = handle;
  return ESP_OK;
}

int adc_driver_read_multi(adc_continuous_handle_t handle,
                          adc_channel_result_t *results,
                          int num_channels) {
  (void)num_channels;

  uint8_t buffer[FRAME_SIZE];
  uint32_t out_len = 0;

  esp_err_t ret = adc_continuous_read(handle, buffer, sizeof(buffer), &out_len, 20);
  if (ret != ESP_OK || out_len == 0) {
    return 0;
  }

#if ADC_USE_PEAK
  uint16_t peak = 0;
  uint32_t count = 0;

  for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= out_len; i += sizeof(adc_digi_output_data_t)) {
    adc_digi_output_data_t sample;
    memcpy(&sample, &buffer[i], sizeof(sample));
    uint16_t v = sample.type1.data;
    if (v > peak) peak = v;
    count++;
  }

  results[0].average = peak;
  return (int)count;
#else
  uint64_t sum = 0;
  uint32_t count = 0;

  for (uint32_t i = 0; i + sizeof(adc_digi_output_data_t) <= out_len; i += sizeof(adc_digi_output_data_t)) {
    adc_digi_output_data_t sample;
    memcpy(&sample, &buffer[i], sizeof(sample));
    sum += sample.type1.data;
    count++;
  }

  results[0].average = count ? (uint16_t)(sum / count) : 0;
  return (int)count;
#endif
}

void adc_driver_deinit(adc_continuous_handle_t handle) {
  if (!handle) {
    ESP_LOGW(TAG, "Handle is NULL");
    return;
  }

  esp_err_t ret = adc_continuous_stop(handle);
  if (ret != ESP_OK) {
    ESP_LOGW(TAG, "adc_continuous_stop: %s", esp_err_to_name(ret));
  }

  ret = adc_continuous_deinit(handle);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "adc_continuous_deinit failed: %s", esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "ADC deinitialized");
  }
}
