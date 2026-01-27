#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_continuous.h"
#include "ssd1306.h"
#include <string.h>


#define TAG "LDR_MONITOR"
#define BUF_SIZE 1024
#define FRAME_SIZE 256
#define SAMPLE_FREQUENCY_HZ 20000

#define I2C_MASTER_SCL_IO 33 //22
#define I2C_MASTER_SDA_IO 32 //21
#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64


static adc_continuous_handle_t adc_config(void);
static uint16_t adc_read(adc_continuous_handle_t);


static void init_oled(SSD1306_t *dev);
static void oled_show(SSD1306_t *dev, float , float );
void draw_box(SSD1306_t *dev, int x, int y, int width, int height, bool invert);
void draw_text_inside_box(SSD1306_t *dev);



// ---------------- TASK ----------------
static void OLED_task(void *pvParameters) {
    //adc_continuous_handle_t adc_handle = adc_config();

    SSD1306_t dev;
    init_oled(&dev);

    while (1) {
    	//poner valores de los sensores
    	float value1 = 5;
    	float value2 = 0.2;
        // OLED
        oled_show(&dev, value1, value2);//aqui pones las nuevas vbles de los sensores
        draw_box(&dev, 10, 10, 108, 25, false);
        draw_text_inside_box(&dev);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void OLED_start(void) {
    xTaskCreate(OLED_task, "OLED_t", 4096, NULL, 5, NULL);
}

// ---------------- FUNCIONES ----------------
static adc_continuous_handle_t adc_config(void) {
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_handle_cfg = {
        .max_store_buf_size = BUF_SIZE,
        .conv_frame_size = FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_handle_cfg, &handle));

    static adc_digi_pattern_config_t adc_pattern = {
        .atten = ADC_ATTEN_DB_12,
        .channel = ADC_CHANNEL_0,
        .unit = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLE_FREQUENCY_HZ,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1,
        .pattern_num = 1,
        .adc_pattern = &adc_pattern,
    };

    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    return handle;
}

static uint16_t adc_read(adc_continuous_handle_t handle) {
    uint8_t buffer[FRAME_SIZE];
    uint32_t out_length = 0;
    uint32_t sum = 0;
    int num_samples = 0;

    esp_err_t ret = adc_continuous_read(handle, buffer, sizeof(buffer),
                                        &out_length, 1000);

    if (ret == ESP_OK) {
        num_samples = out_length / sizeof(adc_digi_output_data_t);

        for (int i = 0; i < out_length; i += sizeof(adc_digi_output_data_t)) {
            adc_digi_output_data_t *p = (adc_digi_output_data_t *)&buffer[i];
            uint16_t value1 = p->type1.data;
            sum += value1;
        }

        return (num_samples > 0) ? (sum / num_samples) : 0;
    } else {
        ESP_LOGW(TAG, "No ADC data read (%s)", esp_err_to_name(ret));
        return 0;
    }
}

static void init_oled(SSD1306_t *dev) {
    i2c_master_init(dev, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, -1);
    ssd1306_init(dev, OLED_WIDTH, OLED_HEIGHT);
    ssd1306_clear_screen(dev, false);
    ssd1306_contrast(dev, 0xff);
}



void draw_box(SSD1306_t *dev, int x, int y, int width, int height, bool invert) {
    
    _ssd1306_line(dev, x, y, x + width, y, invert); 
    _ssd1306_line(dev, x, y + height, x + width, y + height, invert);
    _ssd1306_line(dev, x, y, x, y + height, invert);
    _ssd1306_line(dev, x + width, y, x + width, y + height, invert);

    ssd1306_show_buffer(dev); // refresca la pantalla
}

void draw_text_inside_box(SSD1306_t *dev) {
    const char *text = "DAS";
    int text_len = strlen(text);
    // --- escala de texto x3 ---
    int char_width = 8 * 3;   // cada letra 8px, escala x3
    int char_height = 8 * 3;

    int text_width = char_width * text_len;
    int text_height = char_height;

    //desplazamientos
    int x_offset = 5;
    int y_offset =5;

    // cursor en offset
    _ssd1306_cursor(dev, x_offset, y_offset, 3,false); // escala x3
    ssd1306_display_text_x3(dev, y_offset / 8, text, text_len, false);

    ssd1306_show_buffer(dev); // refresca pantalla
}



static void oled_show(SSD1306_t *dev, float valor1, float valor2) {
  
    ssd1306_clear_screen(dev, false);

    
    char line0[20];
    snprintf(line0, sizeof(line0), " Pulso :%.1f", valor1);
    ssd1306_display_text(dev, 5, line0, strlen(line0), false);

    char line1[20];
    snprintf(line1, sizeof(line1), " MPU :%.1f", valor2);
    ssd1306_display_text(dev, 6, line1, strlen(line1), false);
}
