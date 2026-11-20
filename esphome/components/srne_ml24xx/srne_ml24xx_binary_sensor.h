#pragma once

#include "esphome/core/defines.h"
#ifdef USE_BINARY_SENSOR

#include <cstdint>
#include <span>

#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome::srne_ml24xx {

/// One bit of a register in the live data block
class SrneMl24xxBinarySensor final : public binary_sensor::BinarySensor {
 public:
  SrneMl24xxBinarySensor(uint8_t reg, uint8_t bit) : reg_(reg), bit_(bit) {}

  void on_registers(std::span<const uint16_t> registers) {
    if (this->reg_ < registers.size())
      this->publish_state(((registers[this->reg_] >> this->bit_) & 1) != 0);
  }

 protected:
  const uint8_t reg_;
  const uint8_t bit_;
};

}  // namespace esphome::srne_ml24xx

#endif  // USE_BINARY_SENSOR
