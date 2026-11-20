#pragma once

#include "esphome/core/defines.h"
#ifdef USE_SENSOR

#include "esphome/components/sensor/sensor.h"
#include "srne_ml24xx_registers.h"

namespace esphome::srne_ml24xx {

class SrneMl24xxSensor final : public sensor::Sensor {
 public:
  SrneMl24xxSensor(uint8_t reg, RegisterFormat format, float scale) : reg_(reg), format_(format), scale_(scale) {}

  void on_registers(std::span<const uint16_t> registers) {
    const float value = decode_register(registers, this->reg_, this->format_);
    if (!std::isnan(value))
      this->publish_state(value * this->scale_);
  }

 protected:
  const uint8_t reg_;
  const RegisterFormat format_;
  const float scale_;
};

}  // namespace esphome::srne_ml24xx

#endif  // USE_SENSOR
