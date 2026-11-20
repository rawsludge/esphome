#pragma once

#include <cmath>
#include <cstdint>
#include <span>

namespace esphome::srne_ml24xx {

/// The controller's live data block (0x0100 to 0x0122), read in one request
static constexpr uint16_t RAM_START_ADDRESS = 0x0100;
static constexpr uint16_t RAM_REGISTER_COUNT = 35;
/// High byte: load on (b7) and brightness (b0-b6); low byte: charging state
static constexpr uint8_t REG_LOAD_AND_CHARGING_STATE = 0x20;

enum class RegisterFormat : uint8_t {
  REGISTER_FORMAT_U16,
  /// High word first
  REGISTER_FORMAT_U32,
  REGISTER_FORMAT_TEMPERATURE_HIGH_BYTE,
  REGISTER_FORMAT_TEMPERATURE_LOW_BYTE,
  REGISTER_FORMAT_HIGH_BYTE_LOW_7_BITS,
};

/// b7 is the sign and b0-b6 the magnitude, not two's complement
inline int8_t decode_temperature(uint8_t raw) {
  const auto magnitude = static_cast<int8_t>(raw & 0x7F);
  return (raw & 0x80) != 0 ? static_cast<int8_t>(-magnitude) : magnitude;
}

/// NAN when the register, or the low word of a 32-bit value, is outside `registers`
inline float decode_register(std::span<const uint16_t> registers, uint8_t reg, RegisterFormat format) {
  if (reg >= registers.size())
    return NAN;
  const uint16_t value = registers[reg];
  switch (format) {
    case RegisterFormat::REGISTER_FORMAT_U32:
      if (reg + 1u >= registers.size())
        return NAN;
      return static_cast<float>((static_cast<uint32_t>(value) << 16) | registers[reg + 1]);
    case RegisterFormat::REGISTER_FORMAT_TEMPERATURE_HIGH_BYTE:
      return decode_temperature(value >> 8);
    case RegisterFormat::REGISTER_FORMAT_TEMPERATURE_LOW_BYTE:
      return decode_temperature(value & 0xFF);
    case RegisterFormat::REGISTER_FORMAT_HIGH_BYTE_LOW_7_BITS:
      return (value >> 8) & 0x7F;
    case RegisterFormat::REGISTER_FORMAT_U16:
    default:
      return value;
  }
}

}  // namespace esphome::srne_ml24xx
