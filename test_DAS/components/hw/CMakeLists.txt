idf_component_register(
  SRCS
    "adc.c"
    "pulse_sensor.c"
    "mpu6050.c"
  INCLUDE_DIRS
    "."
  REQUIRES
    freertos
    esp_timer
    driver
    esp_adc
    packet
    esp_driver_i2c
)

