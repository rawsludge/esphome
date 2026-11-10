#include "fridge_device_hub.h"

#ifdef USE_ESP32

namespace esphome::fendt_caravan {
ESPHOME_LOG_TAG(TAG, "raw.fridge");

enum class Key : uint8_t {
  KEY_FRIDGE_AVAILABLE,
  KEY_FRIDGE_ONOFF,
  KEY_FRIDGE_MODE,
  KEY_FRIDGE_SOURCE,
  KEY_FRIDGE_TEMP,
  KEY_FRIDGE_TYPE,
};

static constexpr const char *KEY_NAMES[] = {
    "FRIDGE_AVAILABLE", "FRIDGE_ON_OFF", "FRIDGE_MODE", "FRIDGE_SOURCE", "FRIDGE_TEMP", "FRIDGE_TYPE",
};
// The panel's FRIDGE_MODE value for each select option: Performance, Quiet, Boost
static constexpr uint8_t MODE_DEVICE_VALUES[] = {0, 2, 3};
static constexpr const char *SOURCE_OPTIONS[] = {"Automatic", "Gas", "DirectCurrent", "AlternatingCurrent"};
static constexpr const char *TYPE_OPTIONS[] = {"None", "DometicAbsorberFridge", "HobbyCompressorRMVOC90",
                                               "DOMETICRC104Compressor", "DOMETIC_RUC"};

void FridgeDeviceHub::dump_config() {
  ESP_LOGCONFIG(TAG, " -Fendt Fridge Device-");
  LOG_BINARY_SENSOR(TAG, "  Fridge status", this->fridge_status_binary_sensor_);
  LOG_SWITCH(TAG, "  Fridge on/off", this->fridge_status_switch_);
  LOG_SELECT(TAG, "  Fridge Mode", this->fridge_mode_select_);
  LOG_TEXT_SENSOR(TAG, "  Fridge Source", this->fridge_power_source_text_sensor_);
  LOG_TEXT_SENSOR(TAG, "  Fridge Type", this->fridge_type_text_sensor_);
  LOG_SELECT(TAG, "  Fridge Temperature", this->fridge_temperature_select_);
}

bool FridgeDeviceHub::decode(const char *key, const char *value) {
  size_t i = 0;
  while (i < std::size(KEY_NAMES) && strcmp(KEY_NAMES[i], key) != 0)
    i++;
  if (i == std::size(KEY_NAMES))
    return false;
  ESP_LOGD(TAG, "Handled key: %s, value: %s", key, value);
  Key key_value = static_cast<Key>(i);
  switch (key_value) {
    case Key::KEY_FRIDGE_AVAILABLE: {
      bool fridge_status = DeviceDecoders::decode_bool(value);
      if (this->fridge_status_binary_sensor_)
        this->fridge_status_binary_sensor_->publish_state(fridge_status);
      break;
    }
    case Key::KEY_FRIDGE_ONOFF: {
      bool fridge_onoff = DeviceDecoders::decode_bool(value);
      if (this->fridge_status_switch_)
        this->fridge_status_switch_->publish_state(fridge_onoff);
      break;
    }
    case Key::KEY_FRIDGE_MODE: {
      const int raw = DeviceDecoders::decode_int(value);
      // 1 is also reported as Performance
      const size_t idx = raw <= 1 ? 0 : static_cast<size_t>(raw - 1);
      if (this->fridge_mode_select_ != nullptr && idx < std::size(MODE_DEVICE_VALUES))
        this->fridge_mode_select_->publish_state(idx);
      break;
    }
    case Key::KEY_FRIDGE_SOURCE: {
      // Device sends int index; text_sensor expects string from source list
      int source_idx = DeviceDecoders::decode_int(value);
      if (source_idx >= 0 && source_idx < 4 && this->fridge_power_source_text_sensor_) {
        this->fridge_power_source_text_sensor_->publish_state(SOURCE_OPTIONS[source_idx]);
      }
      break;
    }
    case Key::KEY_FRIDGE_TEMP: {
      const int temp = DeviceDecoders::decode_int(value);
      if (this->fridge_temperature_select_ != nullptr && temp >= 1 && temp <= 5)
        this->fridge_temperature_select_->publish_state(static_cast<size_t>(temp - 1));
      break;
    }
    case Key::KEY_FRIDGE_TYPE: {
      // Device sends int index; text_sensor expects string from type list
      int type_idx = DeviceDecoders::decode_int(value);
      if (type_idx >= 0 && type_idx < 5 && this->fridge_type_text_sensor_) {
        this->fridge_type_text_sensor_->publish_state(TYPE_OPTIONS[type_idx]);
      }
      break;
    }
    default:
      break;
  }
  return true;
}

void FridgeDeviceHub::write_state(switch_::Switch *sw, bool state) {
  if (sw == this->fridge_status_switch_) {
    char cmd[COMMAND_MAX];
    this->parent_->send_command(Commands::update_toggle(cmd, "FRIDGE_ON_OFF"));
  }
}
void FridgeDeviceHub::select_control(select::Select *sel, size_t index) {
  char cmd[COMMAND_MAX];
  if (sel == this->fridge_mode_select_) {
    if (index < std::size(MODE_DEVICE_VALUES))
      this->parent_->send_command(Commands::update_int(cmd, "FRIDGE_MODE", MODE_DEVICE_VALUES[index]));
  } else if (sel == this->fridge_temperature_select_) {
    // Option index 0-4 is temperature level 1-5
    this->parent_->send_command(Commands::update_int(cmd, "FRIDGE_TEMP", index + 1));
  }
}

}  // namespace esphome::fendt_caravan
#endif
