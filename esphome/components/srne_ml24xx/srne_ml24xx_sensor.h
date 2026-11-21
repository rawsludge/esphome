#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SENSOR

#include "esphome/components/sensor/sensor.h"
#include "srne_ml24xx_registers.h"

namespace esphome::srne_ml24xx {

/// A value at an offset inside the register block that starts at `block`
class SrneMl24xxSensor final : public sensor::Sensor {
 public:
  SrneMl24xxSensor(uint16_t block, uint8_t reg, RegisterFormat format, float scale)
      : scale_(scale), block_(block), reg_(reg), format_(format) {}

  uint16_t get_block() const { return this->block_; }
  void on_registers(std::span<const uint16_t> registers);

 protected:
  const float scale_;
  const uint16_t block_;
  const uint8_t reg_;
  const RegisterFormat format_;
};

}  // namespace esphome::srne_ml24xx

#endif  // USE_SENSOR
