#include "OLED.h"
#include "sensor_display.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ssd1306.h"

#define OLED_SDA GPIO_NUM_32
#define OLED_SCL GPIO_NUM_33

#define OLED_WIDTH 128
#define OLED_HEIGHT 64

static SSD1306_t dev;

static void draw_box(SSD1306_t *dev, int x, int y, int width, int height, bool invert) {
  _ssd1306_line(dev, x, y, x + width, y, invert);
  _ssd1306_line(dev, x, y + height, x + width, y + height, invert);
  _ssd1306_line(dev, x, y, x, y + height, invert);
  _ssd1306_line(dev, x + width, y, x + width, y + height, invert);
}

static void draw_title(SSD1306_t *dev) {
  const char *text = "DAS";
  int len = strlen(text);

  _ssd1306_cursor(dev, 5, 5, 3, false);
  ssd1306_display_text_x3(dev, 0, text, len, false);
}

static void oled_show(SSD1306_t *dev, uint16_t pulse, int16_t ax, int16_t ay, int16_t az) {
  char line[32];

  ssd1306_clear_screen(dev, false);

  draw_box(dev, 10, 10, 108, 25, false);
  draw_title(dev);

  snprintf(line, sizeof(line), "Pulso: %u", pulse);
  ssd1306_display_text(dev, 5, line, strlen(line), false);

  snprintf(line, sizeof(line), "AX: %d", ax);
  ssd1306_display_text(dev, 6, line, strlen(line), false);

  snprintf(line, sizeof(line), "AY: %d", ay);
  ssd1306_display_text(dev, 7, line, strlen(line), false);

  snprintf(line, sizeof(line), "AZ: %d", az);
  ssd1306_display_text(dev, 8, line, strlen(line), false);

  ssd1306_show_buffer(dev);
}

static void oled_task(void *arg) {
  uint16_t pulse;
  int16_t ax;
  int16_t ay;
  int16_t az;

  while (1) {
    pulse = sensor_display_get_pulse_bpm();
    sensor_display_get_accel(&ax, &ay, &az);

    oled_show(&dev, pulse, ax, ay, az);

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void OLED_start(void) {
  i2c_master_init(&dev, OLED_SDA, OLED_SCL, -1);
  ssd1306_init(&dev, OLED_WIDTH, OLED_HEIGHT);
  ssd1306_clear_screen(&dev, false);

  xTaskCreate(
    oled_task,
    "oled",
    4096,
    NULL,
    2,
    NULL
  );
}

