#include "../ESP32_WiFi_StandAlone_Test/ApiValidation.h"
using namespace ApiValidation;
constexpr bool accepts(const char *key, const char *text, int expected) {
  Request r;
  return r.add(key, text, 8000, 8) == nullptr && r.values[findField(key)] == expected;
}
constexpr bool rejects(const char *key, const char *text) {
  Request r;
  return r.add(key, text, 8000, 8) != nullptr;
}
constexpr bool duplicateRejected() {
  Request r;
  return !r.add("rpm", "1000", 8000, 8) && r.add("rpm", "2000", 8000, 8) && r.values[Rpm] == 1000;
}
constexpr bool mixedInvalidRejected() {
  Request r;
  return !r.add("run", "1", 8000, 8) && r.add("rpm", "bad", 8000, 8) && r.values[Rpm] == -1;
}
constexpr bool failureDoesNotChangeOutput() {
  uint32_t output = 123;
  return !parseUnsigned("99999999999999999999999999", 8000, output) && output == 123;
}
static_assert(accepts("rpm", "0", 0), "RPM minimum");
static_assert(accepts("rpm", "8000", 8000), "RPM maximum");
static_assert(accepts("rpm", "00050", 50), "Decimal leading zeros");
static_assert(rejects("rpm", "8001"), "RPM range");
static_assert(rejects("rpm", ""), "Empty");
static_assert(rejects("rpm", "-1"), "Negative");
static_assert(rejects("rpm", "+1"), "Plus sign");
static_assert(rejects("rpm", " 1"), "Leading whitespace");
static_assert(rejects("rpm", "1 "), "Trailing whitespace");
static_assert(rejects("rpm", "1.5"), "Fraction");
static_assert(rejects("rpm", "1e3"), "Exponent");
static_assert(rejects("rpm", "100x"), "Trailing junk");
static_assert(rejects("rpm", "4294967296"), "32-bit overflow");
static_assert(failureDoesNotChangeOutput(), "Long overflow leaves output untouched");
static_assert(accepts("run", "0", 0) && accepts("run", "1", 1), "Boolean valid");
static_assert(rejects("run", "2") && rejects("run", "true") && rejects("run", "01"), "Boolean exact encoding");
static_assert(accepts("profile", "0", 0) && accepts("profile", "7", 7), "Profile boundaries");
static_assert(rejects("profile", "8"), "Profile overflow");
static_assert(accepts("tps", "0", 0) && accepts("map", "100", 100), "Sensor boundaries");
static_assert(rejects("ect", "101") && rejects("iat", "-1") && rejects("o2", "foo"), "Sensors malformed");
static_assert(rejects("selftest", "bad") && accepts("selftest", "0", 0), "Self-test validation");
static_assert(rejects("unknown", "1"), "Unknown parameter");
static_assert(duplicateRejected(), "Duplicate rejected without replacing staged value");
static_assert(mixedInvalidRejected(), "Mixed invalid request rejected at staging");

constexpr bool bodyAccepts(const char *body, size_t length, Field field, int expected) {
  Request r;
  return !parseBody(body, length, 8000, 8, r) && r.values[field] == expected;
}
constexpr bool bodyRejectsAtomically(const char *body, size_t length) {
  Request r;
  r.values[Run] = 0;
  r.values[Rpm] = 777;
  if (!parseBody(body, length, 8000, 8, r)) return false;
  if (r.values[Run] != 0 || r.values[Rpm] != 777) return false;
  for (int i = Ckp; i < Count; ++i) if (r.values[i] != -1) return false;
  return true;
}
template<size_t N> constexpr bool bodyRejects(const char (&body)[N]) {
  return bodyRejectsAtomically(body, N - 1);
}
constexpr bool everyFieldAndBoundary() {
  const char body[] = "run=1&rpm=8000&ckp=0&cmp=1&led=0&scope=1&profile=7&selftest=0&tps=0&map=100&ect=0&iat=100&o2=50";
  Request r;
  if (parseBody(body, sizeof(body)-1, 8000, 8, r)) return false;
  const int expected[Count] = {1,8000,0,1,0,1,7,0,0,100,0,100,50};
  for (int i=0;i<Count;++i) if(r.values[i]!=expected[i]) return false;
  return true;
}
constexpr bool allNulPositionsRejected() {
  char body[] = "run=1";
  for(size_t i=0;i<sizeof(body)-1;++i) {
    const char original=body[i];
    body[i]='\0';
    if(!bodyRejectsAtomically(body,sizeof(body)-1)) return false;
    body[i]=original;
  }
  const char suffix[] = {'r','u','n','=','1','\0'};
  return bodyRejectsAtomically(suffix,sizeof(suffix));
}
constexpr bool lengthBoundaries() {
  char body[257]={};
  body[0]='r';body[1]='p';body[2]='m';body[3]='=';
  for(size_t i=4;i<257;++i) body[i]='0';
  return bodyAccepts(body,256,Rpm,0) && bodyRejectsAtomically(body,257);
}
static_assert(everyFieldAndBoundary(), "Full command body / all fields");
static_assert(bodyAccepts("run=0&rpm=0",11,Rpm,0), "Body lower boundaries");
static_assert(bodyRejects(""), "Body empty");
static_assert(bodyRejects("run=1&rpm=bad"), "Mixed invalid body is atomic");
static_assert(bodyRejects("run=1&unknown"), "Bare segment is rejected");
static_assert(bodyRejects("run=1&unknown=1"), "Unknown body key");
static_assert(bodyRejects("run=1&run=0"), "Duplicate body key");
static_assert(bodyRejects("run=1&rpm=8001"), "Body range is atomic");
static_assert(bodyRejects("run=1&&rpm=1000"), "Empty middle segment");
static_assert(bodyRejects("&run=1"), "Empty first segment");
static_assert(bodyRejects("run=1&"), "Trailing separator");
static_assert(bodyRejects("=1"), "Empty key");
static_assert(bodyRejects("rpm="), "Empty body value");
static_assert(bodyRejects("run=1=0"), "Extra equals");
static_assert(bodyRejects("run =1") && bodyRejects("run= 1") && bodyRejects("run=1\n"), "Whitespace");
static_assert(bodyRejects("run=%31") && bodyRejects("run=+1"), "Encoding not allowed");
static_assert(bodyRejects("run=\xFF"), "Non-ASCII");
static_assert(bodyRejects("rpm=999999999999999999999999999"), "Body numeric overflow");
static_assert(bodyRejects("selftest=1&rpm=bad"), "Self-test staged only");
static_assert(allNulPositionsRejected(), "Raw NUL rejected at every position");
static_assert(lengthBoundaries(), "Body cap 256 bytes inclusive");
