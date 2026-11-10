#include "alde_device_hub.h"

#ifdef USE_ESP32
namespace esphome::fendt_caravan {
ESPHOME_LOG_TAG(TAG, "FC.alde");

enum class Key : uint8_t {
  KEY_HEATER_AVAILABLE,
  KEY_HEATER_ONOFF,
  KEY_HEATER_TEMP,
  KEY_HEATER_WATER,
  KEY_HEATER_WATER_TEMP,
  KEY_HEATER_EL,
  KEY_HEATER_GAS,
};

static constexpr const char *KEY_NAMES[] = {
    "HEATER_AVAILABLE", "HEATER_ONOFF", "HEATER_TEMP", "HEATER_WATER", "HEATER_WATER_TEMP", "HEATER_EL", "HEATER_GAS",
};

void AldeDeviceHub::setup() {
  if (this->alde_climate_) {
    this->alde_climate_->add_on_control_callback([this](climate::ClimateCall &call) {
      if (call.get_target_temperature().has_value()) {
        char cmd[COMMAND_MAX];
        this->parent_->send_command(
            Commands::update_temp_10(cmd, "HEATER_TEMP", call.get_target_temperature().value()));
      }
    });
  }
}

void AldeDeviceHub::dump_config() {
  ESP_LOGCONFIG(TAG, " -Fendt Alde Device-");
  LOG_BINARY_SENSOR(TAG, "  Alde Status", this->alde_status_binary_sensor_);
  LOG_CLIMATE(TAG, "  Alde climate", this->alde_climate_);
  LOG_SWITCH(TAG, "  Alde heater switch", this->alde_heater_switch_);
  LOG_SWITCH(TAG, "  Alde heater water", this->alde_water_heater_switch_);
  LOG_SWITCH(TAG, "  Alde heater Water boost", this->alde_water_heater_boost_switch_);
  LOG_SELECT(TAG, "  Heater Electric", this->alde_heater_electricity_select_);
  LOG_SWITCH(TAG, "  Heater Gas", this->alde_heater_gas_switch_);
}

void AldeDeviceHub::write_state(switch_::Switch *sw, bool state) {
  ESP_LOGD(TAG, "Write state called for switch: %p, state: %s", sw, TRUEFALSE(state));
  char cmd[COMMAND_MAX];
  if (sw == this->alde_heater_switch_) {
    this->parent_->send_command(Commands::update_toggle(cmd, "HEATER_ONOFF"));
  } else if (sw == this->alde_water_heater_switch_) {
    this->parent_->send_command(Commands::update_toggle(cmd, "HEATER_WATER"));
  } else if (sw == this->alde_water_heater_boost_switch_) {
    this->parent_->send_command(Commands::update_toggle(cmd, "HEATER_WATER_TEMP"));
  } else if (sw == this->alde_heater_gas_switch_) {
    this->parent_->send_command(Commands::update_run(cmd, "HEATER_GAS"));
  }
}

void AldeDeviceHub::select_control(select::Select *sel, size_t index) {
  if (sel == this->alde_heater_electricity_select_) {
    ESP_LOGV(TAG, "Heater electricity select index: %zu", index);
    char cmd[COMMAND_MAX];
    this->parent_->send_command(Commands::update_net(cmd, "HEATER_EL", index));
  }
}

bool AldeDeviceHub::decode(const char *key, const char *value) {
  size_t i = 0;
  while (i < std::size(KEY_NAMES) && strcmp(KEY_NAMES[i], key) != 0)
    i++;
  if (i == std::size(KEY_NAMES))
    return false;
  ESP_LOGD(TAG, "Handled key: %s, value: %s", key, value);
  Key key_value = static_cast<Key>(i);
  switch (key_value) {
    case Key::KEY_HEATER_AVAILABLE: {
      bool alde_status = DeviceDecoders::decode_bool(value);
      if (this->alde_status_binary_sensor_)
        this->alde_status_binary_sensor_->publish_state(alde_status);
      break;
    }
    case Key::KEY_HEATER_ONOFF: {
      bool alde_heater_onoff = DeviceDecoders::decode_bool(value);
      if (this->alde_heater_switch_)
        this->alde_heater_switch_->publish_state(alde_heater_onoff);
      break;
    }
    case Key::KEY_HEATER_TEMP: {
      float alde_heater_temp = DeviceDecoders::decode_float(value);
      if (this->alde_climate_) {
        this->alde_climate_->target_temperature = alde_heater_temp;
        this->alde_climate_->publish_state();
      }
      break;
    }
    case Key::KEY_HEATER_WATER: {
      bool alde_heater_water = DeviceDecoders::decode_bool(value);
      if (this->alde_water_heater_switch_)
        this->alde_water_heater_switch_->publish_state(alde_heater_water);
      break;
    }
    case Key::KEY_HEATER_WATER_TEMP: {
      float alde_heater_water_temp = DeviceDecoders::decode_float(value);
      if (this->alde_water_heater_boost_switch_)
        this->alde_water_heater_boost_switch_->publish_state(alde_heater_water_temp == 65.0f);
      break;
    }
    case Key::KEY_HEATER_EL: {
      // The panel reports and takes 0-3, the index of "Off", "1 kW", "2 kW", "3 kW"
      const int idx = DeviceDecoders::decode_int(value);
      if (this->alde_heater_electricity_select_ != nullptr && idx >= 0 &&
          static_cast<size_t>(idx) < this->alde_heater_electricity_select_->size())
        this->alde_heater_electricity_select_->publish_state(static_cast<size_t>(idx));
      break;
    }
    case Key::KEY_HEATER_GAS: {
      bool alde_heater_gas = DeviceDecoders::decode_bool(value);
      if (this->alde_heater_gas_switch_)
        this->alde_heater_gas_switch_->publish_state(alde_heater_gas);
      break;
    }
    default:
      break;
  }
  return true;
}

}  // namespace esphome::fendt_caravan
#endif
