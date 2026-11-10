#pragma once

#ifdef USE_ESP32
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace esphome::fendt_caravan {

/// Buffer size for one command, including the terminating NUL
static constexpr size_t COMMAND_MAX = 48;

/// Builds panel commands into a caller-owned buffer of COMMAND_MAX bytes.
/// Each returns `buf`, or nullptr when the command does not fit.
class Commands {
 public:
  static const char *update_toggle(char *buf, const char *name) { return format_command(buf, "cmd-tgl:%s", name); }

  static const char *update_run(char *buf, const char *name) {
    if (strcmp(name, "HS_KEY") == 0 || strcmp(name, "HS_KEY_LONG") == 0)
      return format_command(buf, "net-%s", name);
    return format_command(buf, "cmd-run:%s", name);
  }

  static const char *update_net(char *buf, const char *name, uint16_t value) {
    return check_length(buf, snprintf(buf, COMMAND_MAX, "net-%s-%u", name, static_cast<unsigned>(value)));
  }

  static const char *update_int(char *buf, const char *name, uint16_t value) {
    return check_length(buf, snprintf(buf, COMMAND_MAX, "cmd-set:%s=%u", name, static_cast<unsigned>(value)));
  }

  /// Sends the temperature in tenths of a degree
  static const char *update_temp_10(char *buf, const char *name, float temp) {
    return update_net(buf, name, static_cast<uint16_t>(temp * 10.0f));
  }

 protected:
  static const char *format_command(char *buf, const char *fmt, const char *name) {
    return check_length(buf, snprintf(buf, COMMAND_MAX, fmt, name));
  }
  static const char *check_length(char *buf, int len) {
    return len > 0 && static_cast<size_t>(len) < COMMAND_MAX ? buf : nullptr;
  }
};

}  // namespace esphome::fendt_caravan
#endif
