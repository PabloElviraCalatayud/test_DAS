idf_component_register(
  SRCS 
    "OLED.c"
  INCLUDE_DIRS
    "."
  REQUIRES 
    bt 
    nvs_flash
    esp_adc
    
    
  PRIV_REQUIRES
	sensor_manager driver freertos esp_adc esp_driver_gpio ssd1306
)
