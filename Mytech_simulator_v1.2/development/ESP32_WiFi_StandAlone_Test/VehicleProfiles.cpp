#include "VehicleProfiles.h"

namespace {
const VehicleProfile PROFILES[] = {
  {"Generic", "Hall / Hall", "Custom", TriggerSensorType::Hall, TriggerSensorType::Hall,
   "60-2", "Digital 3-wire sensor template."},
  {"Generic", "VR / Hall", "Custom", TriggerSensorType::MagneticVR, TriggerSensorType::Hall,
   "60-2", "Common mixed sensor template."},
  {"Generic", "VR / VR", "Custom", TriggerSensorType::MagneticVR, TriggerSensorType::MagneticVR,
   "60-2", "Two-wire magnetic sensor template."},
  {"Toyota", "Hilux Vigo", "1KD-FTV (typical)", TriggerSensorType::MagneticVR, TriggerSensorType::Hall,
   "Verify service data", "Configuration varies by production year and ECU."},
  {"Isuzu", "D-Max", "4JJ1 (typical)", TriggerSensorType::MagneticVR, TriggerSensorType::Hall,
   "Verify service data", "Confirm engine code, model year and connector pinout."},
  {"Honda", "Civic FD", "R18A (reference)", TriggerSensorType::Hall, TriggerSensorType::Hall,
   "Verify service data", "Confirm sensor supply and trigger pattern before connection."},
  {"Nissan", "Navara D40", "YD25 (reference)", TriggerSensorType::Hall, TriggerSensorType::Hall,
   "Verify service data", "Confirm engine code and model year before use."},
  {"Mitsubishi", "Triton", "4D56 (reference)", TriggerSensorType::Hall, TriggerSensorType::Hall,
   "Verify service data", "Confirm wiring diagram for the exact ECU."}
};

String jsonEscape(const char *value) {
  String out;
  while (*value) {
    if (*value == '"' || *value == '\\') out += '\\';
    out += *value++;
  }
  return out;
}
}

size_t VehicleProfiles::count() { return sizeof(PROFILES) / sizeof(PROFILES[0]); }

const VehicleProfile &VehicleProfiles::get(size_t index) {
  if (index >= count()) index = 0;
  return PROFILES[index];
}

const char *VehicleProfiles::typeName(TriggerSensorType type) {
  return type == TriggerSensorType::Hall ? "Hall effect" : "Magnetic VR";
}

String VehicleProfiles::toJson() {
  String json = "[";
  for (size_t i = 0; i < count(); ++i) {
    if (i) json += ',';
    const auto &p = PROFILES[i];
    json += "{\"id\":" + String(i);
    json += ",\"brand\":\"" + jsonEscape(p.brand) + "\"";
    json += ",\"model\":\"" + jsonEscape(p.model) + "\"";
    json += ",\"engine\":\"" + jsonEscape(p.engine) + "\"";
    json += ",\"ckpType\":\"" + String(typeName(p.ckpType)) + "\"";
    json += ",\"cmpType\":\"" + String(typeName(p.cmpType)) + "\"";
    json += ",\"ckpPattern\":\"" + jsonEscape(p.ckpPattern) + "\"";
    json += ",\"notes\":\"" + jsonEscape(p.notes) + "\"}";
  }
  return json + "]";
}
