#include "fridge_device_sensor.h"

#ifdef USE_ESP32

namespace esphome::fendt_caravan {
static const char *const TAG = "raw.fridge";

void FridgeDeviceSensor::setup() {
  // if (this->fridge_available_text_sensor_) {
  auto *available = new Variable<std::string>("FRIDGE_AVAILABLE", [](const std::string &value) {
    const char *tmp[] = {"Available", "Not available"};
    return DeviceDecoders::decode_bool_str(value, tmp);
  });
  available->set_on_decode_callback(
      [this](const std::string &value) { this->publish_state(value == "Available" ? 1 : 0); });
  this->add_variable(available);
  //}

  if (this->fridge_status_switch_) {
    auto *status = new Variable<bool>("FRIDGE_ON_OFF", DeviceDecoders::decode_bool, Commands::update_toggle<bool>);
    this->add_variable(status);
  }
  if (this->fridge_mode_select_) {
    std::vector<std::string> list = {"Performance", "Quite", "Boost"};
    auto *fridge_mode = new Variable<std::string>(
        "FRIDGE_MODE",
        [list](const std::string &value) {
          int decoded = DeviceDecoders::decode_int(value);
          if (decoded > 0)
            decoded -= 1;
          return list.at(decoded);
        },
        [list](const std::string &name, std::string value) {
          auto it = std::find(list.begin(), list.end(), value);
          if (it != list.end()) {
            size_t index = std::distance(list.begin(), it);
            if (index > 0)
              index += 1;
            return Commands::update_int(name, index);
          }
          return std::string("");
        });
    this->add_variable(fridge_mode);
  }
  if (this->fridge_source_text_sensor_) {
    std::vector<std::string> list = {"Automatic", "Gas", "DirectCurrent", "AlternatingCurrent"};
    auto *fridge_source = new Variable<std::string>(
        "FRIDGE_SOURCE", [list](const std::string &value) { return DeviceDecoders::decode_int_str(value, list); });
    this->add_variable(fridge_source);
  }
  if (this->fridge_type_text_sensor_) {
    std::vector<std::string> list = {"None", "DometicAbsorberFridge", "HobbyCompressorRMVOC90",
                                     "DOMETICRC104Compressor", "DOMETIC_RUC"};
    auto *fridge_type = new Variable<std::string>(
        "FRIDGE_TYPE", [list](const std::string &value) { return DeviceDecoders::decode_int_str(value, list); });
    this->add_variable(fridge_type);
  }

  if (this->fridge_temperature_select_) {
    auto *fridge_temp = new Variable<std::string>(
        "FRIDGE_TEMP",
        [](const std::string &data) {
          uint16_t value = DeviceDecoders::decode_int(data);
          char buff[5] = {};
          int8_to_str(buff, value);
          return std::string(buff);
        },
        [](const std::string &name, std::string data) {
          uint16_t value = DeviceDecoders::decode_int(data);
          return Commands::update_int(name, value);
        });
    this->add_variable(fridge_temp);
  }
}

void FridgeDeviceSensor::dump_config() {
  ESP_LOGCONFIG(TAG, " -Fendt Fridge Device-");
  LOG_TEXT_SENSOR(TAG, "  Fridge Available", this->fridge_available_text_sensor_);
  LOG_SWITCH(TAG, "  Fridge Status", this->fridge_status_switch_);
  LOG_SELECT(TAG, "  Fridge Mode", this->fridge_mode_select_);
  LOG_TEXT_SENSOR(TAG, "  Fridge Source", this->fridge_source_text_sensor_);
  LOG_TEXT_SENSOR(TAG, "  Fridge Type", this->fridge_type_text_sensor_);
  LOG_SELECT(TAG, "  Fridge Temperature", this->fridge_temperature_select_);
}

void FridgeDeviceSensor::on_state_change_command(const std::string &tag, const std::string &command) {
  if (!command.empty()) {
    ESP_LOGD(TAG, "Switch state changed command:%s", command.c_str());
    this->command_callback_.call(command);
  }
}

}  // namespace esphome::fendt_caravan
#endif
