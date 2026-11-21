#pragma once

#include <cmath>
#include <cstdint>
#include <span>

namespace esphome::srne_ml24xx {

// The protocol does not allow one request to cross these address ranges, so each is read separately.
/// System information (0x000A to 0x001A); fixed, so read until it succeeds once
static constexpr uint16_t SYSTEM_INFO_START_ADDRESS = 0x000A;
static constexpr uint16_t SYSTEM_INFO_REGISTER_COUNT = 17;
/// The controller's live data block (0x0100 to 0x0122)
static constexpr uint16_t RAM_START_ADDRESS = 0x0100;
static constexpr uint16_t RAM_REGISTER_COUNT = 35;
/// EEPROM settings E002 to E021; E001 (charging current limit) can only be written
static constexpr uint16_t SETTINGS_START_ADDRESS = 0xE002;
static constexpr uint16_t SETTINGS_REGISTER_COUNT = 32;
/// The history of day N (0 = today) is the 10 registers at HISTORY_START_ADDRESS | N
static constexpr uint16_t HISTORY_START_ADDRESS = 0xF000;
static constexpr uint16_t HISTORY_REGISTER_COUNT = 10;
static constexpr uint16_t HISTORY_MAX_DAY = 0x03FF;

/// Offsets inside the system information block
static constexpr uint8_t REG_PRODUCT_TYPE = 0x01;
static constexpr uint8_t REG_PRODUCT_MODEL = 0x02;
static constexpr uint8_t PRODUCT_MODEL_REGISTER_COUNT = 8;
static constexpr uint8_t REG_SOFTWARE_VERSION = 0x0A;
static constexpr uint8_t REG_HARDWARE_VERSION = 0x0C;
static constexpr uint8_t REG_SERIAL_NUMBER = 0x0E;

/// Offsets inside the live data block
/// Load on/off command; only acts when the load working mode (E01D) is manual
static constexpr uint8_t REG_LOAD_COMMAND = 0x0A;
/// High byte: load on (b7) and brightness (b0-b6); low byte: charging state
static constexpr uint8_t REG_LOAD_AND_CHARGING_STATE = 0x20;

/// Absolute addresses of settings that are written with a read-modify-write
static constexpr uint16_t SETTING_SYSTEM_VOLTAGE = 0xE003;
static constexpr uint16_t SETTING_SPECIAL_POWER_CONTROL = 0xE021;
static constexpr uint16_t LOAD_COMMAND_ADDRESS = RAM_START_ADDRESS + REG_LOAD_COMMAND;

/// Custom function codes; the request and its echo are the function code followed by 0000 0001
static constexpr uint8_t FUNCTION_FACTORY_RESET = 0x78;
static constexpr uint8_t FUNCTION_CLEAR_HISTORY = 0x79;

/// High byte of E003 (and of 0x000A) for each option of the system voltage select, in option order
static constexpr uint8_t SYSTEM_VOLTAGE_AUTO = 0xFF;
static constexpr uint8_t SYSTEM_VOLTAGES[] = {12, 24, 36, 48, SYSTEM_VOLTAGE_AUTO};
static constexpr uint8_t SYSTEM_VOLTAGE_COUNT = sizeof(SYSTEM_VOLTAGES);

/// The option index of a system voltage byte; SYSTEM_VOLTAGE_COUNT when it is not one of the options
inline uint8_t system_voltage_index(uint8_t volts) {
  for (uint8_t i = 0; i < SYSTEM_VOLTAGE_COUNT; i++) {
    if (SYSTEM_VOLTAGES[i] == volts)
      return i;
  }
  return SYSTEM_VOLTAGE_COUNT;
}

enum class RegisterFormat : uint8_t {
  REGISTER_FORMAT_U16,
  /// High word first
  REGISTER_FORMAT_U32,
  REGISTER_FORMAT_TEMPERATURE_HIGH_BYTE,
  REGISTER_FORMAT_TEMPERATURE_LOW_BYTE,
  REGISTER_FORMAT_HIGH_BYTE_LOW_7_BITS,
  REGISTER_FORMAT_HIGH_BYTE,
  REGISTER_FORMAT_LOW_BYTE,
  /// High byte, with 0xFF (automatic recognition) decoded as NAN
  REGISTER_FORMAT_SYSTEM_VOLTAGE,
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
    case RegisterFormat::REGISTER_FORMAT_HIGH_BYTE:
      return value >> 8;
    case RegisterFormat::REGISTER_FORMAT_LOW_BYTE:
      return value & 0xFF;
    case RegisterFormat::REGISTER_FORMAT_SYSTEM_VOLTAGE:
      return (value >> 8) == SYSTEM_VOLTAGE_AUTO ? NAN : static_cast<float>(value >> 8);
    case RegisterFormat::REGISTER_FORMAT_U16:
    default:
      return value;
  }
}

}  // namespace esphome::srne_ml24xx
