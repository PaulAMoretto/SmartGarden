#pragma once
#include <vector>
#include <memory>
#include <ArduinoJson.h>
#include "../sensors/Sensor.h"

class SensorRegistry {
  std::vector<std::unique_ptr<Sensor>> _sensors;

public:
  template<typename T, typename... Args>
  T* add(Args&&... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T* raw = ptr.get();
    _sensors.push_back(std::move(ptr));
    return raw;
  }

  void readAll(JsonObject& obj) {
    for (auto& s : _sensors) {
      s->read(obj);
    }
  }
};
