#pragma once
#ifdef USE_ESP32
#include "esphome/components/fendt_caravan/caravan_device_component.h"
#include "esphome/components/fendt_caravan/fendt_caravan.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "device_decoders.h"
#include "device_commands.h"

namespace esphome::fendt_caravan {
class FridgeDeviceSensor : public CaravanDeviceComponent, public sensor::Sensor, public Parented<FendtCaravan> {
 public:
  void setup();
  void dump_config();
  void on_state_change_command(const std::string &tag, const std::string &command) override;

  SUB_TEXT_SENSOR(fridge_available);
  SUB_SWITCH(fridge_status);
  SUB_SELECT(fridge_mode);
  SUB_TEXT_SENSOR(fridge_source);
  SUB_TEXT_SENSOR(fridge_type);
  SUB_SELECT(fridge_temperature);

 private:
};

}  // namespace esphome::fendt_caravan
#endif
