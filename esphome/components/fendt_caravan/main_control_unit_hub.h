#pragma once

#ifdef USE_ESP32
//#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/fendt_caravan/variable.h"
#include "esphome/components/sensor/sensor.h"
//#include "esphome/components/switch/switch.h"
#include "esphome/core/string_ref.h"
#include "esphome/core/component.h"
#include "fendt_caravan_hub_base.h"
#include "esphome/core/log.h"
#include "device_decoders.h"
#include "device_commands.h"
#include "fendt_caravan.h"
#include <functional>

namespace esphome::fendt_caravan {

class MainControlUnitHub : public FendtCaravanHubBase, public Parented<FendtCaravan> {
 public:
  void setup() override;
  void dump_config() override;
  void on_state_change_command(const std::string &tag, const std::string &command) override;

  // SUB_SWITCH(main_switch);
  SUB_SENSOR(temp_in);
  SUB_SENSOR(temp_out);
  // SUB_TEXT_SENSOR(power_status);
  // SUB_TEXT_SENSOR(software_version);
  // SUB_SWITCH(all_lights);
  // SUB_SWITCH(floor_heater);

 protected:
  void on_data_decoded(IVariable *variable) override;
};
}  // namespace esphome::fendt_caravan
#endif
