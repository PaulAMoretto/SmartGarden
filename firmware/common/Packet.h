#pragma once
#include <stdint.h>

// Keep this small for ESP-NOW. 250 bytes is a safe target.
static const size_t MAX_JSON = 220;

struct TelemetryPacket {
  uint8_t proto = 1;
  char node_id[16];
  uint32_t seq;
  char json[MAX_JSON];
//Text place holder.
};
