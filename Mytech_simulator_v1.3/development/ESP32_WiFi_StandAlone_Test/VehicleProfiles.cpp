#include "VehicleProfiles.h"

namespace {
using T = TriggerSensorType;

// Each entry drives the real CKP/CMP waveform, not only the UI label.
// verified = tooth and pulse counts come from the cited source. Cam phase
// relative to the CKP gap is still an approximation unless the notes say so;
// confirm with an oscilloscope on a known-good engine before ECU work.
constexpr VehicleProfile PROFILES[] = {
  // ids 0-2 keep their v1.2 meaning: generic 60-2, one 90-degree cam pulse.
  {"Generic", "Hall / Hall (60-2)", "Template", T::Hall, T::Hall,
   {60, 2, 1, {{0, 90}}}, true, "Generic template",
   "Digital 3-wire sensor template."},
  {"Generic", "VR / Hall (60-2)", "Template", T::MagneticVR, T::Hall,
   {60, 2, 1, {{0, 90}}}, true, "Generic template",
   "Common mixed sensor template."},
  {"Generic", "VR / VR (60-2)", "Template", T::MagneticVR, T::MagneticVR,
   {60, 2, 1, {{0, 90}}}, true, "Generic template",
   "Two-wire magnetic sensor template."},
  {"Toyota", "Hilux Vigo / Fortuner / Innova", "1KD-FTV / 2KD-FTV", T::MagneticVR, T::MagneticVR,
   {36, 2, 1, {{335, 20}}}, true,
   "DENSO CRS service manual Hilux/Innova 1KD/2KD (2004): NE 34 pulses/360CA (36-2), G 1 pulse/720CA",
   "ECU detects cylinder 1 when the G pulse and the NE missing-tooth gap occur together; G pulse is placed across the revolution-1 gap."},
  {"Isuzu", "D-Max / MU-7 (common rail)", "4JJ1-TC", T::Hall, T::Hall,
   {60, 4, 5, {{0, 12}, {30, 12}, {180, 12}, {360, 12}, {540, 12}}}, true,
   "Isuzu 4JJ1-TC service manual: CKP 56 notches every 6 deg + 24 deg uncut (60-4); CMP 4 projections every 90 cam deg + 1 reference",
   "CKP/CMP are MRE square-wave sensors. Reference-projection phase is approximate; verify with a scope."},
  {"Mitsubishi", "Triton / Pajero Sport (CRDi)", "4D56 16V Di-D", T::MagneticVR, T::Hall,
   {36, 2, 5, {{0, 10}, {30, 10}, {210, 10}, {390, 10}, {570, 10}}}, false,
   "DENSO HP3 manual L200/Triton 4D56/4M41: TDC sensor 5 pulses/720CA (30, 180, 180, 180 CA spacing). CKP tooth count NOT confirmed",
   "UNVERIFIED CKP: 36-2 is assumed. CMP pulse spacing is from the DENSO manual."},
  {"Honda", "Civic FD", "R18A", T::Hall, T::Hall,
   {60, 2, 1, {{0, 90}}}, false, "No service source yet",
   "UNVERIFIED placeholder (generic 60-2). Fill in the real R18A tooth counts before use."},
  {"Nissan", "Navara D40", "YD25DDTi", T::Hall, T::Hall,
   {60, 2, 1, {{0, 90}}}, false, "No service source yet",
   "UNVERIFIED placeholder (generic 60-2). Fill in the real YD25 tooth counts before use."},
  {"Generic", "VR / Hall (36-1)", "Template", T::MagneticVR, T::Hall,
   {36, 1, 1, {{0, 90}}}, true, "Generic template", "Common 36-1 wheel."},
  {"Generic", "VR / Hall (36-2)", "Template", T::MagneticVR, T::Hall,
   {36, 2, 1, {{0, 90}}}, true, "Generic template", "Common 36-2 wheel."},
};

constexpr bool validPattern(const TriggerPattern &p) {
  if (p.teeth < 4 || p.teeth > MAX_CKP_TEETH || p.missing >= p.teeth / 2) return false;
  if (p.camCount > MAX_CAM_WINDOWS) return false;
  for (uint8_t i = 0; i < p.camCount; ++i) {
    const CamWindow &w = p.cams[i];
    // Every window must be inside 720 degrees and at least one half-tooth wide.
    if (w.startDeg >= 720 || w.widthDeg >= 720 || uint32_t(w.widthDeg) * p.teeth < 180) return false;
  }
  return true;
}

constexpr bool allPatternsValid() {
  for (const auto &p : PROFILES) if (!validPattern(p.pattern)) return false;
  return true;
}
static_assert(allPatternsValid(), "Every vehicle trigger pattern must pass validPattern()");
static_assert(sizeof(PROFILES) / sizeof(PROFILES[0]) <= 256, "API profile index is limited to 0-255");

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

uint16_t VehicleProfiles::buildSlotTable(const TriggerPattern &p, uint8_t (&table)[MAX_TRIGGER_SLOTS]) {
  const uint16_t slots = 4 * p.teeth;
  for (uint16_t s = 0; s < slots; ++s) {
    const uint8_t tooth = (s / 2) % p.teeth;
    table[s] = (s % 2 == 0 && tooth < p.teeth - p.missing) ? 1 : 0;
  }
  for (uint8_t i = 0; i < p.camCount; ++i) {
    // One slot = 180/teeth crank degrees; windows may wrap past 720 degrees.
    const uint16_t start = uint32_t(p.cams[i].startDeg) * p.teeth / 180;
    const uint16_t length = uint32_t(p.cams[i].widthDeg) * p.teeth / 180;
    for (uint16_t k = 0; k < length; ++k) table[(start + k) % slots] |= 2;
  }
  return slots;
}

String VehicleProfiles::toJson() {
  String json = "[";
  for (size_t i = 0; i < count(); ++i) {
    if (i) json += ',';
    const auto &p = PROFILES[i];
    const auto &t = p.pattern;
    json += "{\"id\":" + String(i);
    json += ",\"brand\":\"" + jsonEscape(p.brand) + "\"";
    json += ",\"model\":\"" + jsonEscape(p.model) + "\"";
    json += ",\"engine\":\"" + jsonEscape(p.engine) + "\"";
    json += ",\"ckpType\":\"" + String(typeName(p.ckpType)) + "\"";
    json += ",\"cmpType\":\"" + String(typeName(p.cmpType)) + "\"";
    json += ",\"ckpPattern\":\"" + String(t.teeth) + "-" + String(t.missing) + "\"";
    json += ",\"teeth\":" + String(t.teeth) + ",\"missing\":" + String(t.missing);
    json += ",\"cams\":[";
    for (uint8_t c = 0; c < t.camCount; ++c) {
      if (c) json += ',';
      json += "[" + String(t.cams[c].startDeg) + "," + String(t.cams[c].widthDeg) + "]";
    }
    json += "],\"verified\":" + String(p.verified ? "true" : "false");
    json += ",\"source\":\"" + jsonEscape(p.source) + "\"";
    json += ",\"notes\":\"" + jsonEscape(p.notes) + "\"}";
  }
  return json + "]";
}
