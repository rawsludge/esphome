#pragma once

#ifdef USE_ESP32
#include "esphome/components/fendt_caravan/fendt_caravan_hub_base.h"
#include "esphome/components/switch/switch.h"
#include "esphome/core/string_ref.h"
#include "esphome/core/log.h"

namespace esphome::fendt_caravan {

class FendtSwitch : public switch_::Switch, public Parented<FendtCaravanHubBase> {
 public:
 protected:
  void write_state(bool state) override {
    std::string command;
    if (this->variable_) {
      this->variable_->set_value(state);
      command = this->variable_->get_command();
    }
    this->parent_->on_state_change_command(this->key_name_, command);
  }
  void on_decoded(const bool &value) override { this->publish_state(value); }

 private:
};

}  // namespace esphome::fendt_caravan
#endif
