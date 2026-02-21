#pragma once
#include "Sensor.h"

class FakeAnalog : public Sensor {
  const char* _name;

public:
  explicit FakeAnalog(const char* name) : _name(name) {}

  const char* key() const override { return _name; }

  bool read(JsonObject& out) override {
    // Fake a value that changes so we can see it end-to-end
    static float v = 10.0f;
    v += 0.7f;
    if (v > 99.0f) v = 10.0f;
    out[_name] = v;
    return true;
  }
};
