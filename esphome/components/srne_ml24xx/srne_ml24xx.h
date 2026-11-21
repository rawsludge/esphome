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
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif

namespace esphome::srne_ml24xx {

#ifdef USE_SELECT
/// Publish `index` unless the device reported a value that is not one of the options
inline void publish_select_index(select::Select *select, size_t index) {
  if (select->has_index(index))
    select->publish_state(index);
}
#endif

/// SRNE ML24xx (also sold as Renogy Rover) MPPT solar charge controller on Modbus RTU
class SrneMl24xx final : public PollingComponent, public modbus::ModbusClientDevice {
 public:
  void update() override;
  void dump_config() override;

  void on_read_holding_registers(uint16_t start_address, std::span<const uint16_t> registers,
                                 modbus::ResponseStatus status) override;
  void on_write_single_register(uint16_t address, uint16_t value, modbus::ResponseStatus status) override;
  void on_custom_response(std::span<const uint8_t> request_pdu, std::span<const uint8_t> response_pdu,
                          modbus::ResponseStatus status) override;
  bool on_no_response(std::span<const uint8_t> request_pdu) override;

  /// Read the system information block on the next updates until it has been received once
  void request_system_info() { this->system_info_pending_ = true; }
  /// Read the EEPROM settings block on every update
  void request_settings() { this->read_settings_ = true; }

#ifdef SRNE_ML24XX_SENSOR_COUNT
  void register_sensor(SrneMl24xxSensor *sensor) { this->sensors_.push_back(sensor); }
#endif
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  void register_binary_sensor(SrneMl24xxBinarySensor *sensor) { this->binary_sensors_.push_back(sensor); }
#endif
#ifdef SRNE_ML24XX_HISTORY_DAY_COUNT
  void add_history_day(uint16_t day) { this->history_days_.push_back(day); }
#endif
#ifdef USE_TEXT_SENSOR
  SUB_TEXT_SENSOR(charging_state)
  SUB_TEXT_SENSOR(product_model)
  SUB_TEXT_SENSOR(product_type)
  SUB_TEXT_SENSOR(software_version)
  SUB_TEXT_SENSOR(hardware_version)
  SUB_TEXT_SENSOR(serial_number)
#endif
#ifdef USE_SWITCH
  SUB_SWITCH(load)
#endif
#ifdef USE_SRNE_ML24XX_SETTINGS_PUBLISHER
  /// Publishes the number, select and switch states from the settings block (E002 to E021)
  HUB_PUBLISHER(settings, std::span<const uint16_t>)
#endif

  /// Each returns whether the request was queued
  bool set_load(bool on) { return this->write_single_register(LOAD_COMMAND_ADDRESS, on ? 1 : 0); }
  bool write_setting(uint16_t address, uint16_t value) { return this->write_single_register(address, value); }
  /// `value` in the unit of the entity, `multiplier` turns it into the raw register value
  bool write_scaled_setting(uint16_t address, float value, float multiplier);
  /// Replace the bits in `mask` of a setting that also holds other values; needs one settings read first
  bool write_setting_bits(uint16_t address, uint16_t mask, uint16_t value);
  bool write_system_voltage(size_t index);
  bool factory_reset() { return this->send_command_(FUNCTION_FACTORY_RESET); }
  bool clear_history() { return this->send_command_(FUNCTION_CLEAR_HISTORY); }

 protected:
  bool send_command_(uint8_t function_code);
  bool read_block_(uint16_t start_address, uint16_t count);
  void handle_system_info_(std::span<const uint16_t> registers);
  void handle_ram_(std::span<const uint16_t> registers);
  void handle_settings_(std::span<const uint16_t> registers);
  void publish_sensors_(uint16_t block, std::span<const uint16_t> registers);
  /// The cached value of a setting that write_setting_bits() supports, or nullptr
  uint16_t *cached_setting_(uint16_t address);

#ifdef SRNE_ML24XX_SENSOR_COUNT
  StaticVector<SrneMl24xxSensor *, SRNE_ML24XX_SENSOR_COUNT> sensors_;
#endif
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  StaticVector<SrneMl24xxBinarySensor *, SRNE_ML24XX_BINARY_SENSOR_COUNT> binary_sensors_;
#endif
#ifdef SRNE_ML24XX_HISTORY_DAY_COUNT
  StaticVector<uint16_t, SRNE_ML24XX_HISTORY_DAY_COUNT> history_days_;
#endif
  /// Settings that share a register with other values, kept for write_setting_bits()
  uint16_t system_voltage_setting_{0};
  uint16_t special_power_control_setting_{0};
  bool system_info_pending_ : 1 {false};
  bool read_settings_ : 1 {false};
  bool settings_received_ : 1 {false};
};

}  // namespace esphome::srne_ml24xx
