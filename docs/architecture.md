# SmartGarden Architecture

Sensor Nodes (ESP32) -> ESP-NOW -> Gateway ESP32 (Wi-Fi) -> MQTT -> Raspberry Pi (Mosquitto + Mycodo)

Topics:
- garden/<node_id>/telemetry (JSON)
