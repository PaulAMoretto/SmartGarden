#include <WiFi.h>
#include <esp_now.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#include "core/SensorRegistry.h"
#include "../common/Packet.h"
#include "sensors/FakeAnalog.h"

// ====== CONFIG ======
static const char* NODE_ID = "bed1";

// Gateway MAC address (ESP32 gateway). Fill this in after you read it from gateway Serial.
uint8_t GATEWAY_MAC[6] = {0x10, 0x20, 0xBA, 0x4D, 0xE6, 0x6C};
static const uint8_t ESPNOW_CHANNEL = 6; // TEMP placeholder; we will set this to the gateway WiFi channel
// Publish interval
static const uint32_t PUBLISH_MS = 5000;
// ====================

SensorRegistry sensors;
uint32_t seq_num = 0;

void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  Serial.print("ESP-NOW send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void setupEspNow() {
  WiFi.mode(WIFI_STA); // ESP-NOW uses STA mode
  WiFi.disconnect(true, true);
  delay(100);

  Serial.print("Node WiFi MAC: ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  delay(10);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peerInfo{};
  memcpy(peerInfo.peer_addr, GATEWAY_MAC, 6);
  peerInfo.channel = ESPNOW_CHANNEL;     // same channel as gateway AP/STA
  peerInfo.encrypt = false; // can add encryption later

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add ESP-NOW peer");
    while (true) delay(1000);
  }
}

void setupSensors() {
  // Your desired pattern:
  // sensor1 = sensors.add<SensorType>("name", ...);
  sensors.add<FakeAnalog>("soil_moisture");
  sensors.add<FakeAnalog>("soil_temp_c");
}

void sendTelemetry() {
  StaticJsonDocument<256> doc;
  doc["node_id"] = NODE_ID;
  doc["seq"] = seq_num;

  JsonObject data = doc.createNestedObject("data");
  sensors.readAll(data);

  char jsonBuf[MAX_JSON];
  size_t n = serializeJson(doc, jsonBuf, sizeof(jsonBuf));
  if (n == 0) {
    Serial.println("JSON serialize failed (buffer too small?)");
    return;
  }

  TelemetryPacket pkt{};
  strncpy(pkt.node_id, NODE_ID, sizeof(pkt.node_id) - 1);
  pkt.seq = seq_num++;
  strncpy(pkt.json, jsonBuf, sizeof(pkt.json) - 1);

  esp_err_t result = esp_now_send(GATEWAY_MAC, (uint8_t*)&pkt, sizeof(pkt));
  Serial.print("Sent packet bytes=");
  Serial.print(sizeof(pkt));
  Serial.print(" result=");
  Serial.println(result == ESP_OK ? "ESP_OK" : "ERR");
  Serial.println(pkt.json);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  setupSensors();
  setupEspNow();
}

uint32_t last_pub = 0;

void loop() {
  uint32_t now = millis();
  if (now - last_pub >= PUBLISH_MS) {
    last_pub = now;
    sendTelemetry();
  }
  delay(10);
}
