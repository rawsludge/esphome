#pragma once

#ifdef USE_ESP32

#include "esphome/components/fendt_caravan/fendt_caravan_hub_base.h"
#include "esphome/components/fendt_caravan/variable.h"
#include "esphome/components/select/select.h"
#include "esphome/core/string_ref.h"
#include "esphome/core/log.h"

namespace esphome::fendt_caravan {
class FendtSelect : public select::Select, public Parented<FendtCaravanHubBase> {
 public:
};

}  // namespace esphome::fendt_caravan
#endif
