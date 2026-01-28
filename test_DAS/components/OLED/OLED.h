#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>
#include "../ssd1306/ssd1306.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa la tarea que gestiona el display OLED SSD1306.
 *
 * Crea una tarea FreeRTOS que periódicamente:
 *  - Lee un valor del ADC (o de sensores, en el futuro).
 *  - Calcula el nivel de luz y resistencia.
 *  - Actualiza la pantalla OLED con una barra y valores numéricos.
 *
 * El hardware usa conexión I2C con los pines definidos en OLED.c.
 */
void OLED_start(void);

/**
 * @brief Inicializa el display SSD1306 por I2C.
 *
 * @param dev Puntero a la estructura SSD1306_t.
 */
void init_oled(SSD1306_t *dev);

/**
 * @brief Muestra en la pantalla el nivel de luz y resistencia como texto y barra gráfica.
 *
 * @param dev Puntero al display SSD1306.
 * @param level Nivel de luz (0–99).
 * @param resistance Valor de resistencia del LDR (en ohmios).
 */
void oled_show_light_bar(SSD1306_t *dev, uint8_t level, float resistance);

#ifdef __cplusplus
}
#endif

#endif /* OLED_H_ */
