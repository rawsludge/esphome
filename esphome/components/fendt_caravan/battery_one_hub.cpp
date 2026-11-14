#include "battery_one_hub.h"

#ifdef USE_ESP32
#include "device_decoders.h"
#include "esphome/core/log.h"

namespace esphome::fendt_caravan {
ESPHOME_LOG_TAG(TAG, "fendt_caravan.battery_one");

enum class Key : uint8_t {
  KEY_IBAT,
  KEY_UBAT,
  KEY_SOC,
  KEY_REMAINING_TIME,
  KEY_TEMPERATURE,
  KEY_CAPACITY,
  KEY_AVAILABLE,
  KEY_RECALIBRATED,
  KEY_TYPE,
};

static constexpr const char *KEY_NAMES[] = {
    "IBS0_IBAT",     "IBS0_UBAT",      "IBS0_SOC2",         "IBS0_REMAINING_TIME", "IBS0_TEMPERATURE",
    "IBS0_CAPACITY", "IBS0_AVAILABLE", "IBS0_RECALIBRATED", "IBS0_TYPE",
};
// IBS0_TYPE index; any other value is Unknown
static constexpr const char *TYPE_OPTIONS[] = {"Unknown", "Standard", "Gel", "AGM", "LiFePO4"};

void BatteryOneHub::dump_config() {
  ESP_LOGCONFIG(TAG, "Fendt Battery One");
  LOG_SENSOR(TAG, "  Current", this->battery_current_sensor_);
  LOG_SENSOR(TAG, "  Voltage", this->battery_voltage_sensor_);
  LOG_SENSOR(TAG, "  Level", this->battery_level_sensor_);
  LOG_SENSOR(TAG, "  Remaining Time", this->battery_remaining_time_sensor_);
  LOG_SENSOR(TAG, "  Temperature", this->battery_temperature_sensor_);
  LOG_SENSOR(TAG, "  Capacity", this->battery_capacity_sensor_);
  LOG_BINARY_SENSOR(TAG, "  Available", this->battery_available_binary_sensor_);
  LOG_BINARY_SENSOR(TAG, "  Recalibrated", this->battery_recalibrated_binary_sensor_);
  LOG_TEXT_SENSOR(TAG, "  Type", this->battery_type_text_sensor_);
}

bool BatteryOneHub::decode(const char *key, const char *value) {
  size_t i = 0;
  while (i < std::size(KEY_NAMES) && strcmp(KEY_NAMES[i], key) != 0)
    i++;
  if (i == std::size(KEY_NAMES))
    return false;
  ESP_LOGV(TAG, "Handled key: %s, value: %s", key, value);
  switch (static_cast<Key>(i)) {
    case Key::KEY_IBAT:
      PUBLISH_STATE(this->battery_current_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_UBAT:
      PUBLISH_STATE(this->battery_voltage_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_SOC:
      PUBLISH_STATE(this->battery_level_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_REMAINING_TIME:
      PUBLISH_STATE(this->battery_remaining_time_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_TEMPERATURE:
      PUBLISH_STATE(this->battery_temperature_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_CAPACITY:
      PUBLISH_STATE(this->battery_capacity_sensor_, DeviceDecoders::decode_float(value));
      break;
    case Key::KEY_AVAILABLE:
      PUBLISH_STATE(this->battery_available_binary_sensor_, DeviceDecoders::decode_bool(value));
      break;
    case Key::KEY_RECALIBRATED:
      PUBLISH_STATE(this->battery_recalibrated_binary_sensor_, DeviceDecoders::decode_bool(value));
      break;
    case Key::KEY_TYPE: {
      int idx;
      if (!DeviceDecoders::decode_int(value, idx))
        break;
      if (idx < 0 || static_cast<size_t>(idx) >= std::size(TYPE_OPTIONS))
        idx = 0;
      PUBLISH_STATE(this->battery_type_text_sensor_, TYPE_OPTIONS[idx]);
      break;
    }
  }
  return true;
}

}  // namespace esphome::fendt_caravan
#endif
