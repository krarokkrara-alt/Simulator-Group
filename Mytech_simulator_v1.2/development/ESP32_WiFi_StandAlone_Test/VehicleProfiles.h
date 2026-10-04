#pragma once
#include <Arduino.h>

enum class TriggerSensorType : uint8_t { Hall, MagneticVR };

struct VehicleProfile {
  const char *brand;
  const char *model;
  const char *engine;
  TriggerSensorType ckpType;
  TriggerSensorType cmpType;
  const char *ckpPattern;
  const char *notes;
};

namespace VehicleProfiles {
  size_t count();
  const VehicleProfile &get(size_t index);
  const char *typeName(TriggerSensorType type);
  String toJson();
}
