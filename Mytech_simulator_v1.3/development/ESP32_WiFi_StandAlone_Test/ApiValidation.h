#pragma once
#include <stdint.h>
#include <stddef.h>

// Pure validation: no GPIO, timer or SimulatorState side effects.
namespace ApiValidation {
enum Field { Run, Rpm, Ckp, Cmp, Led, Scope, Profile, SelfTest, Tps, Map, Ect, Iat, O2, Count };
constexpr const char *names[Count] = {
  "run", "rpm", "ckp", "cmp", "led", "scope", "profile", "selftest", "tps", "map", "ect", "iat", "o2"
};

constexpr bool same(const char *a, const char *b) {
  if (!a || !b) return false;
  while (*a && *a == *b) { ++a; ++b; }
  return *a == *b;
}

constexpr int findField(const char *name) {
  for (int i = 0; i < Count; ++i) if (same(name, names[i])) return i;
  return -1;
}

// Decimal ASCII digits only. Overflow/range checks happen before multiplication.
constexpr bool parseUnsigned(const char *text, uint32_t maximum, uint32_t &value) {
  if (!text || !*text) return false;
  uint32_t parsed = 0;
  while (*text) {
    if (*text < '0' || *text > '9') return false;
    const uint32_t digit = uint32_t(*text++ - '0');
    if (parsed > maximum / 10 || (parsed == maximum / 10 && digit > maximum % 10)) return false;
    parsed = parsed * 10 + digit;
  }
  value = parsed;
  return true;
}

struct Request {
  int values[Count]; // -1 means omitted; all validated values are nonnegative.
  constexpr Request() : values{} {
    for (int i = 0; i < Count; ++i) values[i] = -1;
  }
  constexpr const char *add(const char *name, const char *text, uint16_t maxRpm, size_t profileCount) {
    const int field = findField(name);
    if (field < 0) return "unknown parameter";
    if (values[field] != -1) return "duplicate parameter";
    const bool boolean = field == Run || field == Ckp || field == Cmp || field == Led || field == Scope || field == SelfTest;
    if (boolean && !same(text, "0") && !same(text, "1")) return "boolean must be 0 or 1";
    if (field == Profile && (profileCount == 0 || profileCount > 256)) return "invalid profile catalog";
    const uint32_t maximum = field == Rpm ? maxRpm : field == Profile ? uint32_t(profileCount - 1) : boolean ? 1 : 100;
    uint32_t parsed = 0;
    if (!parseUnsigned(text, maximum, parsed)) return "invalid integer or value out of range";
    values[field] = int(parsed);
    return nullptr;
  }
};

constexpr size_t BodyLimit = 256;
// Input has an explicit byte length. Do not URL-decode or use C-string length.
// Assign the caller's request only after the entire body has passed validation.
constexpr const char *parseBody(const char *body, size_t length, uint16_t maxRpm, size_t profileCount, Request &output) {
  if (!body || length == 0) return "empty body";
  if (length > BodyLimit) return "body exceeds 256 bytes";
  char buffer[BodyLimit + 1] = {};
  for (size_t i = 0; i < length; ++i) {
    if (body[i] < '!' || body[i] > '~') return "body must be ASCII without whitespace or NUL";
    buffer[i] = body[i];
  }
  Request staged;
  size_t start = 0;
  while (start < length) {
    size_t end = start;
    while (end < length && buffer[end] != '&') ++end;
    if (end == start) return "empty body segment";
    size_t equals = start;
    while (equals < end && buffer[equals] != '=') ++equals;
    if (equals == start || equals == end || equals + 1 == end) return "each segment must be key=digits";
    buffer[equals] = '\0';
    buffer[end] = '\0';
    const char *error = staged.add(buffer + start, buffer + equals + 1, maxRpm, profileCount);
    if (error) return error;
    if (end < length && end + 1 == length) return "trailing separator";
    start = end + 1;
  }
  output = staged;
  return nullptr;
}
}
