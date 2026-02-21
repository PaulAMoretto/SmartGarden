#pragma once
#include <ArduinoJson.h>

class Sensor {
public:
  virtual const char* key() const = 0;
  virtual bool read(JsonObject& out) = 0;  // set out[key]=value
  virtual ~Sensor() = default;
};
