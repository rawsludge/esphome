#pragma once

#ifdef USE_ESP32
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "fendt_caravan.h"
#include "fendt_caravan_hub_base.h"

namespace esphome::fendt_caravan {

/// The first battery sensor (IBS0) on the panel's bus
class BatteryOneHub : public FendtCaravanHubBase, public Parented<FendtCaravan> {
 public:
  void dump_config() override;
  bool decode(const char *key, const char *value) override;

  SUB_SENSOR(battery_current);
  SUB_SENSOR(battery_voltage);
  SUB_SENSOR(battery_level);
  SUB_SENSOR(battery_remaining_time);
  SUB_SENSOR(battery_temperature);
  SUB_SENSOR(battery_capacity);
  SUB_BINARY_SENSOR(battery_available);
  SUB_BINARY_SENSOR(battery_recalibrated);
  SUB_TEXT_SENSOR(battery_type);
};

}  // namespace esphome::fendt_caravan
#endif
