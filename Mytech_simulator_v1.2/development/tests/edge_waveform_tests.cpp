#include "../ESP32_WiFi_StandAlone_Test/EdgeWaveform.h"
using namespace EdgeWaveform;
// Deliberately synthetic irregular CKP and multi-pulse CMP, not an OEM profile.
constexpr Edge irregular[] = {{0,CKP,CKP},{5000,CKP,0},{90000,CMP,CMP},{120000,CKP,CKP},{180000,Channels,0},{400000,CMP,CMP},{410000,CMP,0},{600000,CKP,CKP},{710000,CKP,0}};
constexpr Table table{irregular,9,0,CMP};
static_assert(validate(table)==Error::None && validateTiming(table,8000)==Error::None,"Irregular wheel and two cam pulses accepted");
constexpr unsigned risingCount(uint8_t channel){unsigned count=0;uint8_t previous=table.initialLevels;for(size_t i=0;i<table.count;++i){const auto next=apply(previous,table.edges[i]);if(!(previous&channel)&&(next&channel))++count;previous=next;}return count;}
static_assert(risingCount(CKP)==3 && risingCount(CMP)==2,"Unequal CKP spacing and multiple CMP pulses are preserved");
constexpr uint32_t intervalSum(uint16_t rpm){uint32_t sum=0;for(size_t i=0;i<table.count;++i)sum+=intervalUs(table,i,cycleUs(rpm));return sum;}
static_assert(intervalSum(1)==120000000 && intervalSum(8000)==15000 && intervalSum(7777)==cycleUs(7777),"Rounded deadlines telescope through full wrap");
static_assert(deadlineUs(0,cycleUs(8000))==0 && intervalUs(table,8,15000)==208,"Angle-zero immediate event and last-to-first wrap");
static_assert(physical(0,CMP)==CMP && physical(Channels,CMP)==CKP,"Per-channel polarity inversion");
constexpr Edge narrow[]={{0,CKP,CKP},{1,CKP,0}};
static_assert(validateTiming({narrow,2,0,0},8000)==Error::Resolution,"Submicrosecond transitions rejected rather than stretching cycle");
constexpr Edge nearZero[]={{1,CKP,CKP},{360000,CKP,0}};
constexpr Edge exactZero[]={{0,CKP,CKP},{360000,CKP,0}};
static_assert(validateTiming({nearZero,2,0,0},8000)==Error::Resolution,"Positive first phase cannot arm a zero-us timer");
static_assert(validateTiming({exactZero,2,0,0},8000)==Error::None,"Exact phase zero is a synchronous edge, not a timer alarm");
constexpr Edge duplicate[]={{10,CKP,CKP},{10,CKP,0}};
constexpr Edge seam[]={{10,CKP,CKP}};
constexpr Edge redundant[]={{10,CKP,0}};
constexpr Edge badMask[]={{10,CKP,CMP}};
constexpr Edge outOfCycle[]={{CycleTicks,CKP,CKP}};
static_assert(validate({duplicate,2,0,0})==Error::Angle,"Simultaneous events require one merged edge");
static_assert(validate({seam,1,0,0})==Error::Seam,"No hidden wrap transition");
static_assert(validate({redundant,1,0,0})==Error::Redundant,"Every requested channel must transition");
static_assert(validate({badMask,1,0,0})==Error::Mask,"Levels cannot affect an unselected channel");
static_assert(validate({outOfCycle,1,0,0})==Error::Angle,"720-degree endpoint excluded");
static_assert(validate({nullptr,1,0,0})==Error::Storage && validate({irregular,0,0,0})==Error::Storage && validate({irregular,257,0,0})==Error::Storage,"Bounded caller storage");
static_assert(validate({irregular,9,4,0})==Error::Mask && validate({irregular,9,0,4})==Error::Mask,"Initial and polarity masks bounded");
static_assert(validateTiming(table,0)==Error::Rpm && validateTiming(table,8001)==Error::Rpm,"RPM range enforced");
constexpr Edge offset[]={{100000,CKP,0},{600000,CKP,CKP}};
static_assert(validateTiming({offset,2,CKP,CKP},8000)==Error::None,"Nonzero first angle and initially high logical channel");
static_assert(deadlineUs(offset[0].angle,15000)==2083,"First timer delay respects phase");
int main(){return 0;}
