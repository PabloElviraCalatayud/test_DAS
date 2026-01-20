idf_component_register(
  SRCS
    "ota_manager.c"
  INCLUDE_DIRS
    "."
  REQUIRES
    system
    ble
    sensor_manager
  PRIV_REQUIRES app_update
)

