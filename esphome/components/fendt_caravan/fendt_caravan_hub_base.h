#pragma once

#ifdef USE_ESP32
#include "esphome/core/component.h"

namespace esphome::fendt_caravan {

class FendtCaravanHubBase : public PollingComponent {
 public:
  virtual bool decode(const char *name, const char *value) { return false; }
  void dump_config() override = 0;
  // Most hubs publish as reports arrive and have nothing to poll
  void update() override {}
};

}  // namespace esphome::fendt_caravan
#endif
