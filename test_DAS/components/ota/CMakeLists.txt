idf_component_register(
  SRCS "ota_manager.c"
  INCLUDE_DIRS "."
  REQUIRES 
    ble
  PRIV_REQUIRES app_update
)

