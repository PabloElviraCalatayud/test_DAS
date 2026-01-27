idf_component_register(
  SRCS
    main.c
  INCLUDE_DIRS
    .
  REQUIRES
    ota
    hw
    ble
    sensor_test
    packet
    freertos
    system
    sensor_manager
  PRIV_REQUIRES
    driver freertos esp_adc esp_driver_gpio OLED
    
)

