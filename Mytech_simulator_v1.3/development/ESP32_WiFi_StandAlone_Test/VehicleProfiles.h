#pragma once
#include <Arduino.h>

enum class TriggerSensorType : uint8_t { Hall, MagneticVR };

// One CMP high window inside the 720-degree cycle. Angles are crank degrees
// after the first CKP tooth that follows the missing-tooth gap (revolution 1).
struct CamWindow {
  uint16_t startDeg;
  uint16_t widthDeg;
};

constexpr uint8_t MAX_CAM_WINDOWS = 6;
constexpr uint8_t MAX_CKP_TEETH = 60;
// Half-tooth slots per 720 degrees: 2 revolutions x 2 edges x teeth.
constexpr uint16_t MAX_TRIGGER_SLOTS = 4 * MAX_CKP_TEETH;

struct TriggerPattern {
  uint8_t teeth;    // Tooth positions per crank revolution, including missing ones.
  uint8_t missing;  // Consecutive missing teeth at the end of each revolution.
  uint8_t camCount;
  CamWindow cams[MAX_CAM_WINDOWS];
};

struct VehicleProfile {
  const char *brand;
  const char *model;
  const char *engine;
  TriggerSensorType ckpType;
  TriggerSensorType cmpType;
  TriggerPattern pattern;
  bool verified;       // Tooth/pulse counts taken from a cited service source.
  const char *source;
  const char *notes;
};

namespace VehicleProfiles {
  size_t count();
  const VehicleProfile &get(size_t index);
  const char *typeName(TriggerSensorType type);
  // Fills one byte per half-tooth slot: bit0 = CKP high, bit1 = CMP high.
  // Returns the slot count for 720 degrees.
  uint16_t buildSlotTable(const TriggerPattern &pattern, uint8_t (&table)[MAX_TRIGGER_SLOTS]);
  String toJson();
}
