#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "esp_crc.h"
#include "esp_adc_cal.h"
#define __HOME__
// #define __OFFICE__
#include "types.h"

// #define _DEV1_
#define _DEV2_

#define POWER_LED_PIN 26
#define NOW_LED_PIN   25

#ifdef _DEV1_
#include <DHT.h>
#define DHT11_PIN     32

DHT dht(DHT11_PIN, DHT11);

typedef struct dht_sensor {
  float temperature = 0;
  float humidity    = 0;
} dht_value_t;

dht_value_t dht_value;
#endif

#ifdef _DEV2_
#define GUVA_PIN      33
#define LDR_PIN       32
esp_adc_cal_characteristics_t *adc_chars;
float guva_value = 0;
uint8_t ldr_value = 0;
#endif

// --- Common variables ---
void get_sensor_data(void);
volatile bool has_data = false;
esp_now_packet_t gateway_packet;
static bool handshaked = false;
static bool is_getting_value = false;
static uint32_t interval = 2000;

void printPacket(const esp_now_packet_t &pkt);

// --- ESP-NOW callbacks ---
void esp_now_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Data sent successfully" : "Failed to send data");
}

void esp_now_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (memcmp(mac_addr, gateway.peer_addr, 6) == 0) {
    if (data_len == sizeof(esp_now_packet_t)) {
      memcpy(&gateway_packet, data, sizeof(gateway_packet));
      has_data = true;
    } else {
      Serial.println("Data size mismatch from Gateway");
    }
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(POWER_LED_PIN, OUTPUT);
  pinMode(NOW_LED_PIN, OUTPUT);
  digitalWrite(NOW_LED_PIN, HIGH); // Power ON

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("STA MAC: ");
  Serial.println(WiFi.macAddress());

  memcpy(gateway.peer_addr, gateway_broadcast_address, 6);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }
  esp_now_register_send_cb(esp_now_send_cb);
  esp_now_register_recv_cb(esp_now_recv_cb);

  if (esp_now_add_peer(&gateway) == ESP_OK)
    Serial.println("Gateway peer added");
  else
    Serial.println("Failed to add gateway peer");

#ifdef _DEV1_
  dht.begin();
#endif

#ifdef _DEV2_
  analogSetPinAttenuation(GUVA_PIN, ADC_0db);  // dải đo 0–1.1V
  analogReadResolution(12);

  adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, adc_chars);

  pinMode(LDR_PIN, INPUT);
#endif
}

void loop() {
  static esp_now_packet_t packet;
  static esp_now_packet_t packet_return;
  static unsigned long lastAckTime = 0;
  static unsigned long lastSendTime = 0;

  if (has_data) {
    if (memcmp(gateway_packet.packet_type, PACKET_TYPE_HANDSHAKE, sizeof(gateway_packet.packet_type)) == 0) {
      Serial.println("Processing HANDSHAKE...");
      memset(&packet, 0, sizeof(packet));
      memcpy(packet.packet_type, PACKET_TYPE_ACK, sizeof(packet.packet_type));
      esp_now_send(gateway.peer_addr, (uint8_t *)&packet, sizeof(packet));
    } 
    else if (memcmp(gateway_packet.packet_type, PACKET_TYPE_ACK, sizeof(gateway_packet.packet_type)) == 0) {
      Serial.println("Got confirm ACK");
      handshaked = true;
      lastAckTime = millis();
    } 
    else if (memcmp(gateway_packet.packet_type, PACKET_TYPE_GET_VALUE, sizeof(gateway_packet.packet_type)) == 0) {
      Serial.println("Processing GET_VALUE...");
      is_getting_value = true;

      get_sensor_data();
      memset(&packet_return, 0, sizeof(packet_return));
      memcpy(packet_return.packet_type, PACKET_TYPE_GET_RETURN, sizeof(packet_return.packet_type));

#ifdef _DEV1_
      snprintf((char *)packet_return.data, sizeof(packet_return.data),
               "T=%.1fC,H=%.1f%%", dht_value.temperature, dht_value.humidity);
#endif
#ifdef _DEV2_
      snprintf((char *)packet_return.data, sizeof(packet_return.data),
               "UV=%.2f,LDR=%d", guva_value, ldr_value);
#endif
      packet_return.crc = esp_crc16_le(0, packet_return.data, sizeof(packet_return.data));

      esp_now_send(gateway.peer_addr, (uint8_t *)&packet_return, sizeof(packet_return));
      Serial.println("Sent GET_RETURN data");

      is_getting_value = false;
    } else if (memcmp(gateway_packet.packet_type, PACKET_TYPE_SET_INTERVAL, sizeof(gateway_packet.packet_type)) == 0) {
      interval = atoi((char *)gateway_packet.data);
      Serial.printf("Set new interval: %d ms\n", interval);
    }

    has_data = false;
  }

  // Gửi data định kỳ khi handshaked và không GET_VALUE
  if (handshaked && !is_getting_value) {
    unsigned long now = millis();
    if (now - lastSendTime >= interval) {
      get_sensor_data();
      memset(&packet, 0, sizeof(packet));
      memcpy(packet.packet_type, PACKET_TYPE_DATA, sizeof(packet.packet_type));

#ifdef _DEV1_
      snprintf((char *)packet.data, sizeof(packet.data),
               "T=%.1f,H=%.1f", dht_value.temperature, dht_value.humidity);
#endif
#ifdef _DEV2_
      snprintf((char *)packet.data, sizeof(packet.data),
               "UV=%.2f,LDR=%d", guva_value, ldr_value);
#endif
      packet.crc = esp_crc16_le(0, packet.data, sizeof(packet.data));
      esp_now_send(gateway.peer_addr, (uint8_t *)&packet, sizeof(packet));

      lastSendTime = now;
    }

    if (now - lastAckTime > GATEWAY_TIMEOUT) {
      Serial.println("No ACK for 10s — handshake lost!");
      handshaked = false;
    }
  }

  if (handshaked) {
    digitalWrite(NOW_LED_PIN, LOW);
  } else {
    digitalWrite(NOW_LED_PIN, HIGH);
  }
}

// --- Debug ---
void printPacket(const esp_now_packet_t &pkt) {
  Serial.print("packet_type: ");
  for (int i = 0; i < 3; i++) Serial.printf("%c", pkt.packet_type[i]);
  Serial.println();
  Serial.print("data: ");
  for (int i = 0; i < 50; i++) {
    if (pkt.data[i] == 0) break;
    Serial.write(pkt.data[i]);
  }
  Serial.println();
  Serial.printf("crc: %u (0x%04X)\n", pkt.crc, pkt.crc);
}

// --- Sensor reading ---
void get_sensor_data(void) {
#ifdef _DEV1_
  dht_value.temperature = dht.readTemperature();
  dht_value.humidity = dht.readHumidity();
#endif

#ifdef _DEV2_
  int raw = analogRead(GUVA_PIN);
  uint32_t mv = esp_adc_cal_raw_to_voltage(raw, adc_chars);
  float voltage = mv / 1000.0f;
  guva_value = voltage / 0.1f;  // mW/cm² gần đúng
  ldr_value = !digitalRead(LDR_PIN);
#endif
}
