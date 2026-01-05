idf_component_register(
  SRCS
    "packet_manager.c"
  INCLUDE_DIRS
    "."
  REQUIRES
    ble
    freertos
    esp_timer
)

