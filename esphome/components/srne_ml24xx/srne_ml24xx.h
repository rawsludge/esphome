#pragma once

#include <span>

#include "esphome/components/modbus/modbus.h"
#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/helpers.h"
#include "srne_ml24xx_registers.h"

#ifdef USE_SENSOR
#include "srne_ml24xx_sensor.h"
#endif
#ifdef USE_BINARY_SENSOR
#include "srne_ml24xx_binary_sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif

namespace esphome::srne_ml24xx {

/// SRNE ML24xx (also sold as Renogy Rover) MPPT solar charge controller on Modbus RTU
class SrneMl24xx final : public PollingComponent, public modbus::ModbusClientDevice {
 public:
  void update() override;
  void dump_config() override;

  void on_read_holding_registers(uint16_t start_address, std::span<const uint16_t> registers,
                                 modbus::ResponseStatus status) override;
  bool on_no_response(std::span<const uint8_t> request_pdu) override;

#ifdef SRNE_ML24XX_SENSOR_COUNT
  void register_sensor(SrneMl24xxSensor *sensor) { this->sensors_.push_back(sensor); }
#endif
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  void register_binary_sensor(SrneMl24xxBinarySensor *sensor) { this->binary_sensors_.push_back(sensor); }
#endif
#ifdef USE_TEXT_SENSOR
  void set_charging_state_text_sensor(text_sensor::TextSensor *sensor) { this->charging_state_text_sensor_ = sensor; }
#endif

 protected:
#ifdef SRNE_ML24XX_SENSOR_COUNT
  StaticVector<SrneMl24xxSensor *, SRNE_ML24XX_SENSOR_COUNT> sensors_;
#endif
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  StaticVector<SrneMl24xxBinarySensor *, SRNE_ML24XX_BINARY_SENSOR_COUNT> binary_sensors_;
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *charging_state_text_sensor_{nullptr};
#endif
};

}  // namespace esphome::srne_ml24xx
