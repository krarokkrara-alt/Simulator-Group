#include "../ESP32_WiFi_StandAlone_Test/EdgeWaveform.h"
using namespace EdgeWaveform;
constexpr Edge nearOrigin[]={{1,CKP,CKP},{360000,CKP,0}};
static_assert(validateTiming({nearOrigin,2,0,0},8000)==Error::Resolution,
              "A positive first angle cannot arm a zero-us initial alarm");
constexpr Edge simultaneous[]={{0,Channels,Channels},{360000,Channels,0}};
static_assert(validateTiming({simultaneous,2,0,Channels},8000)==Error::None,
              "Both channels may transition together with inverted physical levels");
constexpr Edge partiallyRedundant[]={{0,Channels,CKP},{360000,CKP,0}};
static_assert(validate({partiallyRedundant,2,0,0})==Error::Redundant,
              "Selected channels must each change, not merely one selected channel");
constexpr Edge tailRoundsToCycle[]={{0,CKP,CKP},{719999,CKP,0}};
static_assert(validateTiming({tailRoundsToCycle,2,0,0},8000)==Error::Resolution,
              "An event rounding to the endpoint cannot disappear at wrap");
constexpr Edge decreasing[]={{400000,CKP,CKP},{300000,CKP,0}};
static_assert(validate({decreasing,2,0,0})==Error::Angle,"Reject decreasing angles");
int main(){return 0;}
