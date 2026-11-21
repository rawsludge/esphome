#include "srne_ml24xx.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "esphome/core/log.h"
#include "esphome/core/progmem.h"

namespace esphome::srne_ml24xx {

ESPHOME_LOG_TAG(TAG, "srne_ml24xx");

#ifdef USE_TEXT_SENSOR
// Indexed by the low byte of register 0x0120; the last entry is the fallback
PROGMEM_STRING_TABLE(ChargingStateStrings, "Deactivated", "Activated", "MPPT", "Equalizing", "Boost", "Floating",
                     "Current limiting", "Unknown");
// Indexed by the low byte of register 0x000B; the last entry is the fallback
PROGMEM_STRING_TABLE(ProductTypeStrings, "Controller", "Inverter", "Unknown");

template<typename Table> static void publish_table_entry(text_sensor::TextSensor *sensor, uint8_t index) {
  char name[Table::MAX_LENGTH + 1];
  ESPHOME_strncpy_P(name, reinterpret_cast<const char *>(Table::get_progmem_str(index, Table::LAST_INDEX)),
                    sizeof(name));
  name[sizeof(name) - 1] = '\0';
  ESP_LOGD(TAG, "'%s' published %s", LOG_STR_ARG(sensor->get_log_name()), name);
  sensor->publish_state(name);
}

/// The lowest byte is unused; 00 01 04 00 is V01.04.00
static void publish_version(text_sensor::TextSensor *sensor, std::span<const uint16_t> registers, uint8_t reg) {
  char version[sizeof("V255.255.255")];
  snprintf(version, sizeof(version), "V%02u.%02u.%02u", registers[reg] & 0xFF, registers[reg + 1] >> 8,
           registers[reg + 1] & 0xFF);
  sensor->publish_state(version);
}
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
  ESP_LOGV(TAG, "'%s' published %s", LOG_STR_ARG(this->get_log_name()), ONOFF(state));
  this->publish_state(state);
}
#endif

bool SrneMl24xx::read_block_(uint16_t start_address, uint16_t count) {
  if (this->read_holding_registers(start_address, count))
    return true;
  ESP_LOGW(TAG, "Could not queue the read of 0x%04X", start_address);
  this->status_set_warning();
  return false;
}

void SrneMl24xx::update() {
  this->read_block_(RAM_START_ADDRESS, RAM_REGISTER_COUNT);
  if (this->system_info_pending_)
    this->read_block_(SYSTEM_INFO_START_ADDRESS, SYSTEM_INFO_REGISTER_COUNT);
  if (this->read_settings_)
    this->read_block_(SETTINGS_START_ADDRESS, SETTINGS_REGISTER_COUNT);
#ifdef SRNE_ML24XX_HISTORY_DAY_COUNT
  for (uint16_t day : this->history_days_)
    this->read_block_(HISTORY_START_ADDRESS | day, HISTORY_REGISTER_COUNT);
#endif
}

static uint16_t expected_register_count(uint16_t start_address) {
  switch (start_address) {
    case SYSTEM_INFO_START_ADDRESS:
      return SYSTEM_INFO_REGISTER_COUNT;
    case RAM_START_ADDRESS:
      return RAM_REGISTER_COUNT;
    case SETTINGS_START_ADDRESS:
      return SETTINGS_REGISTER_COUNT;
    default:
      return (start_address & ~HISTORY_MAX_DAY) == HISTORY_START_ADDRESS ? HISTORY_REGISTER_COUNT : 0;
  }
}

void SrneMl24xx::on_read_holding_registers(uint16_t start_address, std::span<const uint16_t> registers,
                                           modbus::ResponseStatus status) {
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "Read of 0x%04X failed with exception 0x%02X", start_address, static_cast<uint8_t>(*status));
    this->status_set_warning();
    return;
  }
  if (registers.size() != expected_register_count(start_address)) {
    ESP_LOGW(TAG, "Unexpected response: %u registers from 0x%04X", static_cast<unsigned>(registers.size()),
             start_address);
    this->status_set_warning();
    return;
  }
  this->status_clear_warning();
  ESP_LOGV(TAG, "Received %u registers from 0x%04X", static_cast<unsigned>(registers.size()), start_address);

  switch (start_address) {
    case SYSTEM_INFO_START_ADDRESS:
      this->handle_system_info_(registers);
      break;
    case RAM_START_ADDRESS:
      this->handle_ram_(registers);
      break;
    case SETTINGS_START_ADDRESS:
      this->handle_settings_(registers);
      break;
    default:
      break;
  }
  this->publish_sensors_(start_address, registers);
}

void SrneMl24xx::publish_sensors_(uint16_t block, std::span<const uint16_t> registers) {
#ifdef SRNE_ML24XX_SENSOR_COUNT
  for (auto *sensor : this->sensors_) {
    if (sensor->get_block() == block)
      sensor->on_registers(registers);
  }
#endif
}

void SrneMl24xx::handle_system_info_(std::span<const uint16_t> registers) {
  this->system_info_pending_ = false;
#ifdef USE_TEXT_SENSOR
  if (this->product_model_text_sensor_ != nullptr) {
    // ASCII, padded with spaces or NULs on either side
    char model[PRODUCT_MODEL_REGISTER_COUNT * 2 + 1];
    for (uint8_t i = 0; i < PRODUCT_MODEL_REGISTER_COUNT; i++) {
      model[i * 2] = static_cast<char>(registers[REG_PRODUCT_MODEL + i] >> 8);
      model[i * 2 + 1] = static_cast<char>(registers[REG_PRODUCT_MODEL + i] & 0xFF);
    }
    size_t end = sizeof(model) - 1;
    while (end > 0 && (model[end - 1] == ' ' || model[end - 1] == '\0'))
      end--;
    model[end] = '\0';
    size_t start = 0;
    while (start < end && model[start] == ' ')
      start++;
    this->product_model_text_sensor_->publish_state(model + start);
  }
  if (this->product_type_text_sensor_ != nullptr)
    publish_table_entry<ProductTypeStrings>(this->product_type_text_sensor_, registers[REG_PRODUCT_TYPE] & 0xFF);
  if (this->software_version_text_sensor_ != nullptr)
    publish_version(this->software_version_text_sensor_, registers, REG_SOFTWARE_VERSION);
  if (this->hardware_version_text_sensor_ != nullptr)
    publish_version(this->hardware_version_text_sensor_, registers, REG_HARDWARE_VERSION);
  if (this->serial_number_text_sensor_ != nullptr) {
    char serial[sizeof("FFFFFFFF")];
    snprintf(serial, sizeof(serial), "%04X%04X", registers[REG_SERIAL_NUMBER], registers[REG_SERIAL_NUMBER + 1]);
    this->serial_number_text_sensor_->publish_state(serial);
  }
#endif
}

void SrneMl24xx::handle_ram_(std::span<const uint16_t> registers) {
#ifdef SRNE_ML24XX_BINARY_SENSOR_COUNT
  for (auto *sensor : this->binary_sensors_)
    sensor->on_registers(registers);
#endif
#ifdef USE_TEXT_SENSOR
  if (this->charging_state_text_sensor_ != nullptr)
    publish_table_entry<ChargingStateStrings>(this->charging_state_text_sensor_,
                                              registers[REG_LOAD_AND_CHARGING_STATE] & 0xFF);
#endif
#ifdef USE_SWITCH
  if (this->load_switch_ != nullptr)
    this->load_switch_->publish_state(registers[REG_LOAD_COMMAND] != 0);
#endif
}

void SrneMl24xx::handle_settings_(std::span<const uint16_t> registers) {
  this->system_voltage_setting_ = registers[SETTING_SYSTEM_VOLTAGE - SETTINGS_START_ADDRESS];
  this->special_power_control_setting_ = registers[SETTING_SPECIAL_POWER_CONTROL - SETTINGS_START_ADDRESS];
  this->settings_received_ = true;
#ifdef USE_SRNE_ML24XX_SETTINGS_PUBLISHER
  if (this->settings_publisher_ != nullptr)
    this->settings_publisher_(registers);
#endif
}

bool SrneMl24xx::write_scaled_setting(uint16_t address, float value, float multiplier) {
  const long raw = lroundf(value * multiplier);
  return this->write_setting(address, static_cast<uint16_t>(std::clamp<long>(raw, 0, UINT16_MAX)));
}

uint16_t *SrneMl24xx::cached_setting_(uint16_t address) {
  switch (address) {
    case SETTING_SYSTEM_VOLTAGE:
      return &this->system_voltage_setting_;
    case SETTING_SPECIAL_POWER_CONTROL:
      return &this->special_power_control_setting_;
    default:
      return nullptr;
  }
}

bool SrneMl24xx::write_setting_bits(uint16_t address, uint16_t mask, uint16_t value) {
  uint16_t *cached = this->cached_setting_(address);
  if (cached == nullptr) {
    ESP_LOGW(TAG, "Setting 0x%04X does not support partial writes", address);
    return false;
  }
  if (!this->settings_received_) {
    ESP_LOGW(TAG, "Setting 0x%04X not written: the settings have not been read yet", address);
    return false;
  }
  const uint16_t merged = (*cached & ~mask) | (value & mask);
  if (!this->write_setting(address, merged))
    return false;
  // Kept so a second change before the next read does not undo this one
  *cached = merged;
  return true;
}

bool SrneMl24xx::write_system_voltage(size_t index) {
  if (index >= SYSTEM_VOLTAGE_COUNT)
    return false;
  return this->write_setting_bits(SETTING_SYSTEM_VOLTAGE, 0xFF00, SYSTEM_VOLTAGES[index] << 8);
}

bool SrneMl24xx::send_command_(uint8_t function_code) {
  const uint8_t pdu[] = {function_code, 0x00, 0x00, 0x00, 0x01};
  if (this->queue_pdu(pdu))
    return true;
  ESP_LOGW(TAG, "Could not queue command 0x%02X", function_code);
  return false;
}

void SrneMl24xx::on_write_single_register(uint16_t address, uint16_t value, modbus::ResponseStatus status) {
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "Writing 0x%04X to 0x%04X failed with exception 0x%02X", value, address,
             static_cast<uint8_t>(*status));
    return;
  }
  ESP_LOGD(TAG, "Wrote 0x%04X to 0x%04X", value, address);
}

void SrneMl24xx::on_custom_response(std::span<const uint8_t> request_pdu, std::span<const uint8_t> response_pdu,
                                    modbus::ResponseStatus status) {
  const uint8_t function_code = request_pdu.empty() ? 0 : request_pdu[0];
  const LogString *name = function_code == FUNCTION_FACTORY_RESET   ? LOG_STR("Factory reset")
                          : function_code == FUNCTION_CLEAR_HISTORY ? LOG_STR("Clear history")
                                                                    : LOG_STR("Command");
  if (!modbus::succeeded(status)) {
    ESP_LOGW(TAG, "%s failed with exception 0x%02X", LOG_STR_ARG(name), static_cast<uint8_t>(*status));
    return;
  }
  // The controller confirms by echoing the request
  if (!std::equal(request_pdu.begin(), request_pdu.end(), response_pdu.begin(), response_pdu.end())) {
    ESP_LOGW(TAG, "%s: unexpected response", LOG_STR_ARG(name));
    return;
  }
  ESP_LOGI(TAG, "%s done", LOG_STR_ARG(name));
}

bool SrneMl24xx::on_no_response(std::span<const uint8_t> request_pdu) {
  if (request_pdu.size() >= 3) {
    ESP_LOGW(TAG, "No response from address 0x%02X to function 0x%02X at 0x%04X", this->address_, request_pdu[0],
             encode_uint16(request_pdu[1], request_pdu[2]));
  } else {
    ESP_LOGW(TAG, "No response from address 0x%02X", this->address_);
  }
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
