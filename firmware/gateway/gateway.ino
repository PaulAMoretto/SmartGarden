#include <WiFi.h>
#include <esp_now.h>
#include <PubSubClient.h>

#include "config/secrets.h"
#include "../common/Packet.h"

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

static const char* GATEWAY_ID = "gw1";

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK. IP=");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect failed");
  }
}

void ensureMqtt() {
  if (mqtt.connected()) return;

  mqtt.setServer(MQTT_HOST, MQTT_PORT);

  String clientId = String("smartgarden-") + GATEWAY_ID;

  Serial.print("Connecting MQTT to ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  bool ok;
  if (String(MQTT_USER).length() > 0) {
    ok = mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS);
  } else {
    ok = mqtt.connect(clientId.c_str());
  }

  if (ok) {
    Serial.println("MQTT connected");
  } else {
    Serial.print("MQTT connect failed rc=");
    Serial.println(mqtt.state());
  }
}

void publishTelemetry(const TelemetryPacket& pkt, int rssi) {
  // Publish raw JSON under a stable topic
  String topic = String("garden/") + pkt.node_id + "/telemetry";

  // Optional: wrap/augment could be done here later.
  // For now, publish exactly what node sends.
  bool ok = mqtt.publish(topic.c_str(), pkt.json);
  Serial.print("MQTT publish ");
  Serial.print(topic);
  Serial.print(" ok=");
  Serial.println(ok ? "true" : "false");

  // Also publish a tiny status topic (optional)
  String st = String("{\"gateway_id\":\"") + GATEWAY_ID + "\",\"rssi\":" + rssi + "}";
  mqtt.publish((String("garden/") + pkt.node_id + "/status").c_str(), st.c_str());
}

// ESP-NOW receive callback
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != (int)sizeof(TelemetryPacket)) {
    Serial.print("RX unexpected len=");
    Serial.println(len);
    return;
  }

  TelemetryPacket pkt{};
  memcpy(&pkt, data, sizeof(pkt));
  if (pkt.proto != 1) {
    Serial.print("RX wrong proto=");
    Serial.println(pkt.proto);
    return;
  }
  int rssi = info->rx_ctrl ? info->rx_ctrl->rssi : 0;

  Serial.print("RX from ");
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);
  Serial.print(macStr);
  Serial.print(" rssi=");
  Serial.print(rssi);
  Serial.print(" node=");
  Serial.print(pkt.node_id);
  Serial.print(" seq=");
  Serial.println(pkt.seq);
  Serial.println(pkt.json);

  if (mqtt.connected()) {
    publishTelemetry(pkt, rssi);
  }
}

void setupEspNowRx() {
  // ESP-NOW needs STA mode (can still be on Wi-Fi)
  WiFi.mode(WIFI_STA);

  Serial.print("Gateway WiFi MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onRecv);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  ensureWifi();
  setupEspNowRx();
  ensureMqtt();
}

void loop() {
  ensureWifi();
  ensureMqtt();
  mqtt.loop();
  delay(10);
}
