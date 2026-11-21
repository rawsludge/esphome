#include "srne_ml24xx.h"
#include "esphome/core/log.h"
#include "esphome/core/progmem.h"

namespace esphome::srne_ml24xx {

ESPHOME_LOG_TAG(TAG, "srne_ml24xx");

#ifdef USE_TEXT_SENSOR
// Indexed by the low byte of register 0x0120; the last entry is the fallback
PROGMEM_STRING_TABLE(ChargingStateStrings, "Deactivated", "Activated", "MPPT", "Equalizing", "Boost", "Floating",
                     "Current limiting", "Unknown");
#endif

#ifdef USE_SENSOR
void SrneMl24xxSensor::on_registers(std::span<const uint16_t> registers) {
  const float value = decode_register(registers, this->reg_, this->format_);
  if (std::isnan(value))
    return;
  const float state = value * this->scale_;
  ESP_LOGD(TAG, "'%s' published %f", LOG_STR_ARG(this->get_log_name()), state);
  this->publish_state(state);
}
#endif

#ifdef USE_BINARY_SENSOR
void SrneMl24xxBinarySensor::on_registers(std::span<const uint16_t> registers) {
  if (this->reg_ >= registers.size())
    return;
  const bool state = ((registers[this->reg_] >> this->bit_) & 1) != 0;
  ESP_LOGD(TAG, "'%s' published %s", LOG_STR_ARG(this->get_log_name()), ONOFF(state));
  this->publish_state(state);
}
#endif

void SrneMl24xx::update() {
  if (!this->read_holding_registers(RAM_START_ADDRESS, RAM_REGISTER_COUNT)) {
    ESP_LOGW(TAG, "Could not queue the read request");
    this->status_set_warning();
    return;
  }
  ESP_LOGD(TAG, "Requested %u registers from 0x%04X", static_cast<unsigned>(RAM_REGISTER_COUNT),
           static_cast<unsigned>(RAM_START_ADDRESS));
}

void SrneMl24xx::on_read_holding_registers(uint16_t start_address, std::span<const uint16_t> registers,
                                           modbus::ResponseStatus status) {
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "Read failed with exception 0x%02X", static_cast<uint8_t>(*status));
    this->status_set_warning();
    return;
  }
  if (start_address != RAM_START_ADDRESS || registers.size() != RAM_REGISTER_COUNT) {
    ESP_LOGW(TAG, "Unexpected response: %u registers from 0x%04X", static_cast<unsigned>(registers.size()),
             start_address);
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();
  ESP_LOGD(TAG, "Received %u registers from 0x%04X", static_cast<unsigned>(registers.size()),
           static_cast<unsigned>(start_address));

#ifdef SRNE_ML24XX_SENSOR_COUNT
  for (auto *sensor : this->sensors_)
    sensor->on_registers(registers);
#endif
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  for (auto *sensor : this->binary_sensors_)
    sensor->on_registers(registers);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->charging_state_text_sensor_ != nullptr) {
    const uint8_t state = registers[REG_LOAD_AND_CHARGING_STATE] & 0xFF;
    char name[ChargingStateStrings::MAX_LENGTH + 1];
    ESPHOME_strncpy_P(
        name,
        reinterpret_cast<const char *>(ChargingStateStrings::get_progmem_str(state, ChargingStateStrings::LAST_INDEX)),
        sizeof(name));
    name[sizeof(name) - 1] = '\0';
    ESP_LOGD(TAG, "Charging state: %s", name);
    this->charging_state_text_sensor_->publish_state(name);
  }
#endif
}

bool SrneMl24xx::on_no_response(std::span<const uint8_t> request_pdu) {
  ESP_LOGW(TAG, "No response from address 0x%02X", this->address_);
  this->status_set_warning();
  return false;
}

void SrneMl24xx::dump_config() {
  ESP_LOGCONFIG(TAG,
                "SRNE ML24xx:\n"
                "  Address: 0x%02X",
                this->address_);
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace esphome::srne_ml24xx
