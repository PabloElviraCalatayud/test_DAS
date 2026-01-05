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
)

