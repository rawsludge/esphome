#pragma once

#ifdef USE_ESP32
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/select/select.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/string_ref.h"
#include "fendt_caravan_hub_base.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "device_decoders.h"
#include "device_commands.h"
#include "fendt_caravan.h"
#include "variable.h"

namespace esphome::fendt_caravan {

using namespace std;

#define SUB_CLIMATE(name) \
 protected: \
  climate::Climate *name##_climate_{nullptr}; \
\
 public: \
  void set_##name##_climate(climate::Climate *c) { this->name##_climate_ = c; }

class AldeDeviceSensor : public CaravanDeviceComponent, public sensor::Sensor, public Parented<FendtCaravan> {
 public:
  void setup() override;
  void dump_config() override;
  void update() override;

  SUB_BINARY_SENSOr(alde_status);
  SUB_CLIMATE(alde_climate);
  SUB_SWITCH(alde_heater);
  SUB_SWITCH(alde_heater_water);
  SUB_SWITCH(alde_heater_water_temperature);
  SUB_SELECT(alde_heater_electricity);
  SUB_SWITCH(alde_heater_gas);
};
}  // namespace esphome::fendt_caravan
#endif
