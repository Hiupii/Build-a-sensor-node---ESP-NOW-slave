#ifndef TYPES_H
#define TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <Arduino.h>
#include <esp_now.h>

#ifdef __RELEASE__
#define WIFI_SSID "Vanchang2003"
#define WIFI_PASSWORD "123456789"
#endif

#ifdef __HOME__
#define WIFI_SSID "ぼっち・ざ・ろっく"
#define WIFI_PASSWORD "22222222"
#endif

#ifdef __OFFICE__
#define WIFI_SSID "ヒエウ"
#define WIFI_PASSWORD "123456789"
#endif

#define PACKET_TYPE_HANDSHAKE     "HSK"
#define PACKET_TYPE_ACK           "ACK"
#define PACKET_TYPE_SET_INTERVAL  "SIN"
#define PACKET_TYPE_GET_VALUE     "GVL"
#define PACKET_TYPE_DATA          "DAT"
#define PACKET_TYPE_GET_RETURN    "GRT"

#define GATEWAY_TIMEOUT  10000

esp_now_peer_info_t gateway;
uint8_t gateway_broadcast_address[] = {0xF4, 0x65, 0x0B, 0xBF, 0xB5, 0xD0};

typedef struct __attribute__((packed)) {
  uint8_t packet_type[3];
  uint8_t data[50];
  uint16_t crc;
} esp_now_packet_t;

#ifdef __cplusplus
}
#endif

#endif      // __TYPES__