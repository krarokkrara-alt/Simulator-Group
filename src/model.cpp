#include "model.h"

const char* healthText(HealthLevel level) {
  switch (level) {
    case HealthLevel::PASS: return "PASS";
    case HealthLevel::WARN: return "WARN";
    case HealthLevel::FAIL: return "FAIL";
    default: return "NOT_TESTED";
  }
}

