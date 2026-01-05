#pragma once

#include <stdint.h>

#define PACKET_VERSION  0x01
#define PKT_TYPE_DATA   0x01

#define PACKET_IMU_MAX    6
#define PACKET_PULSE_MAX  4

typedef struct __attribute__((packed)) {
  uint8_t  version;
  uint8_t  type;
  uint8_t  imu_count;
  uint8_t  pulse_count;
  uint32_t ts_ms;
} packet_header_t;

typedef struct __attribute__((packed)) {
  int16_t ax;
  int16_t ay;
  int16_t az;
  int16_t gx;
  int16_t gy;
  int16_t gz;
} packet_imu_raw_t;

typedef uint16_t packet_pulse_raw_t;

typedef struct __attribute__((packed)) {
  packet_header_t header;
  packet_imu_raw_t imu[PACKET_IMU_MAX];
  packet_pulse_raw_t pulse[PACKET_PULSE_MAX];
} packet_t;

_Static_assert(sizeof(packet_t) == 88, "packet_t must be 88 bytes");

