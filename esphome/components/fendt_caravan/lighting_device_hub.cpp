#include "lighting_device_hub.h"

#ifdef USE_ESP32
namespace esphome::fendt_caravan {
ESPHOME_LOG_TAG(TAG, "fendt_caravan.lights");

void LightingDeviceHub::dump_config() { ESP_LOGCONFIG(TAG, "-Fendt Lighting Device-"); }

void LightingDeviceHub::set_binary_light_state(const char *key_name, light::LightState *state) {
#ifdef LIGHTING_DEVICE_BINARY_LIGHT_COUNT
  this->binary_states_.push_back({StringRef(key_name), state});
#endif
}

void LightingDeviceHub::set_float_light_state(const char *key_name, light::LightState *state) {
#ifdef LIGHTING_DEVICE_FLOAT_LIGHT_COUNT
  this->float_states_.push_back({StringRef(key_name), state});
#endif
}

bool LightingDeviceHub::decode(const char *key, const char *value) {
#ifdef LIGHTING_DEVICE_BINARY_LIGHT_COUNT
  for (auto &entry : this->binary_states_) {
    if (entry.key == key) {
      ESP_LOGD(TAG, "Handled key: %s, value: %s", key, value);
      int decoded_value;
      if (!DeviceDecoders::decode_int(value, decoded_value))
        return true;
      bool cur_state = decoded_value != 0;
      entry.is_on = cur_state;
      entry.state->remote_values.set_state(cur_state);
      entry.state->publish_state();
      return true;
    }
  }
#endif  // LIGHTING_DEVICE_BINARY_LIGHT_COUNT
#ifdef LIGHTING_DEVICE_FLOAT_LIGHT_COUNT
  for (auto &entry : this->float_states_) {
    if (entry.key == key) {
      ESP_LOGD(TAG, "Handled key: %s, value: %s", key, value);
      int decoded_value;
      if (!DeviceDecoders::decode_int(value, decoded_value))
        return true;
      bool cur_state = decoded_value != 0;
      entry.is_on = cur_state;
      entry.state->remote_values.set_state(cur_state);
      entry.state->remote_values.set_brightness((float) decoded_value / 15.0f);
      entry.state->publish_state();
      return true;
    }
  }
#endif  // LIGHTING_DEVICE_FLOAT_LIGHT_COUNT
  return false;
}
void LightingDeviceHub::write_state(const char *key_name, light::LightState *state) {
  char cmd[COMMAND_MAX];
#ifdef LIGHTING_DEVICE_BINARY_LIGHT_COUNT
  for (auto &entry : this->binary_states_) {
    if (entry.key == key_name) {
      if (entry.is_on != state->remote_values.is_on()) {
        this->parent_->send_command(Commands::update_toggle(cmd, entry.key.c_str()));
      }
      return;
    }
  }
#endif
#ifdef LIGHTING_DEVICE_FLOAT_LIGHT_COUNT
  for (auto &entry : this->float_states_) {
    if (entry.key == key_name) {
      if (entry.is_on != state->remote_values.is_on()) {
        this->parent_->send_command(Commands::update_toggle(cmd, entry.key.c_str()));
      } else {
        if (state->remote_values.is_on()) {
          float brightness = state->remote_values.get_brightness();
          this->parent_->send_command(Commands::update_int(cmd, entry.key.c_str(), (int) (brightness * 15.0f)));
        }
      }
      return;
    }
  }
#endif
}

}  // namespace esphome::fendt_caravan
#endif
