#pragma once
#ifdef USE_ESP32
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "fendt_caravan_hub_base.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "device_decoders.h"
#include "device_commands.h"
#include "fendt_caravan.h"
#include "variable.h"

namespace esphome::fendt_caravan {
class FridgeDeviceHub : public FendtCaravanHubBase, public Parented<FendtCaravan> {
 public:
  void setup();
  void dump_config();
  void decode(IVariable *variable) override;

  SUB_BINARY_SENSOR(fridge_status);
  SUB_SWITCH(fridge_status);
  SUB_SELECT(fridge_mode);
  SUB_TEXT_SENSOR(fridge_power_source);
  SUB_TEXT_SENSOR(fridge_type);
  SUB_SELECT(fridge_temperature);

 private:
};

}  // namespace esphome::fendt_caravan
#endif
