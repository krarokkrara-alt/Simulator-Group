#pragma once
#include <stdint.h>
#include <stddef.h>

// Pure candidate engine; not connected to live timers or an OEM catalogue.
namespace EdgeWaveform {
constexpr uint32_t CycleTicks = 720000; // 1000 ticks per crank degree; 720 degrees.
constexpr uint8_t CKP = 1, CMP = 2, Channels = CKP | CMP;
constexpr size_t MaxEdges = 256;
struct Edge { uint32_t angle; uint8_t channels; uint8_t levels; };
// initialLevels is the logical state immediately before angle zero.
// polarity bits invert logical levels into physical pin levels.
// Storage is caller-owned and must remain immutable/alive during use.
struct Table { const Edge *edges; size_t count; uint8_t initialLevels; uint8_t polarity; };
enum class Error : uint8_t { None, Storage, Mask, Angle, Redundant, Seam, Rpm, Resolution };

constexpr uint8_t apply(uint8_t previous, const Edge &edge) {
  return uint8_t((previous & ~edge.channels) | edge.levels);
}
constexpr uint8_t physical(uint8_t logical, uint8_t polarity) { return (logical ^ polarity) & Channels; }
constexpr Error validate(const Table &table) {
  // Empty tables are intentionally rejected: no signal is not a waveform.
  if (!table.edges || table.count == 0 || table.count > MaxEdges) return Error::Storage;
  if ((table.initialLevels | table.polarity) & ~Channels) return Error::Mask;
  uint8_t levels = table.initialLevels;
  for (size_t i = 0; i < table.count; ++i) {
    const auto &edge = table.edges[i];
    if (!edge.channels || (edge.channels & ~Channels) || (edge.levels & ~edge.channels)) return Error::Mask;
    if (edge.angle >= CycleTicks || (i && edge.angle <= table.edges[i - 1].angle)) return Error::Angle;
    const auto next = apply(levels, edge);
    if (((levels ^ next) & edge.channels) != edge.channels) return Error::Redundant;
    levels = next;
  }
  // No implicit seam transition: every changed channel is explicitly listed.
  return levels == table.initialLevels ? Error::None : Error::Seam;
}
constexpr uint32_t cycleUs(uint16_t rpm) {
  return rpm && rpm <= 8000 ? uint32_t((120000000ULL + rpm / 2) / rpm) : 0;
}
// Cumulative rounded deadlines preserve the rounded cycle duration: do not
// independently round each interval or clamp a short interval to 1 us.
constexpr uint32_t deadlineUs(uint32_t angle, uint32_t cycle) {
  return uint32_t((uint64_t(angle) * cycle + CycleTicks / 2) / CycleTicks);
}
constexpr uint32_t intervalUs(const Table &table, size_t index, uint32_t cycle) {
  const uint32_t current = deadlineUs(table.edges[index].angle, cycle);
  return index + 1 < table.count ? deadlineUs(table.edges[index + 1].angle, cycle) - current
                              : cycle - current + deadlineUs(table.edges[0].angle, cycle);
}
constexpr Error validateTiming(const Table &table, uint16_t rpm) {
  const auto result = validate(table);
  if (result != Error::None) return result;
  const auto cycle = cycleUs(rpm);
  if (!cycle) return Error::Rpm;
  if (table.edges[0].angle != 0 && deadlineUs(table.edges[0].angle, cycle) == 0) return Error::Resolution;
  for (size_t i = 0; i < table.count; ++i)
    if (deadlineUs(table.edges[i].angle, cycle) >= cycle || intervalUs(table, i, cycle) == 0) return Error::Resolution;
  return Error::None;
}
// Integration contract: validateTiming before publishing a table. Apply initial
// physical levels at START; apply an angle-zero edge synchronously, never arm a
// zero-us alarm. Otherwise first alarm is deadlineUs(first.angle, cycle).
// After each edge schedule intervalUs(). STOP must use explicit pin LOW, not
// physical(initialLevels, polarity), because inverted inactive levels can be HIGH.
// No helper here allocates, performs GPIO writes, or claims ISR/cache safety.
}
