file(TO_CMAKE_PATH "$ENV{IDF_PATH}" IDF_PATH_CMAKE)

idf_component_register(
  SRCS
    bluetooth.c
  INCLUDE_DIRS
    .
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/esp-hci/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/nimble/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/nimble/transport/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/porting/nimble/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/port/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/porting/npl/freertos/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/port/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/nimble/host/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/nimble/host/services/gap/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/porting/nimble/include"
    "${IDF_PATH_CMAKE}/components/bt/host/nimble/nimble/nimble/host/services/gatt/include"
  REQUIRES
    nvs_flash
    bt
    ota
    system
)

