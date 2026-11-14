#include "device_decoders.h"
#include <cmath>
#include <cstdlib>
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome::fendt_caravan {
ESPHOME_LOG_TAG(TAG, "fendt_caravan.decoders");

// Copies `value` without surrounding whitespace into `buffer`; false when it does not fit
static bool copy_trimmed(const char *value, char *buffer, size_t size) {
  while (std::isspace(static_cast<unsigned char>(*value)))
    value++;
  size_t len = strlen(value);
  while (len > 0 && std::isspace(static_cast<unsigned char>(value[len - 1])))
    len--;
  if (len >= size)
    return false;
  memcpy(buffer, value, len);
  buffer[len] = '\0';
  return true;
}

// Units the panel appends to a value ("20,0^C", "12,6 V", "90%"); '°' may come as UTF-8 or Latin-1.
// A longer unit comes before any unit it ends with.
static constexpr const char *UNITS[] = {"\xC2\xB0"
                                        "C",
                                        "\xB0"
                                        "C",
                                        "^C",
                                        "Ah",
                                        "%",
                                        "V",
                                        "A",
                                        "h"};

// Removes one trailing unit from UNITS and the whitespace before it
static void strip_unit(char *buffer) {
  size_t len = strlen(buffer);
  for (const char *unit : UNITS) {
    const size_t unit_len = strlen(unit);
    if (len <= unit_len || memcmp(buffer + len - unit_len, unit, unit_len) != 0)
      continue;
    len -= unit_len;
    while (len > 0 && std::isspace(static_cast<unsigned char>(buffer[len - 1])))
      len--;
    buffer[len] = '\0';
    return;
  }
}

float DeviceDecoders::decode_float(const char *data) {
  char buffer[32];
  if (!copy_trimmed(data, buffer, sizeof(buffer))) {
    ESP_LOGW(TAG, "Value too long for a number: '%s'", data);
    return NAN;
  }
  strip_unit(buffer);
  for (char *p = buffer; *p != '\0'; p++) {
    if (*p == ',')
      *p = '.';
  }
  auto val = parse_number<float>(buffer);
  if (!val.has_value() || !std::isfinite(*val)) {
    ESP_LOGW(TAG, "Not a number: '%s'", data);
    return NAN;
  }
  return *val;
}

bool DeviceDecoders::decode_int(const char *value, int &out) {
  char buffer[16];
  optional<int> val;
  if (copy_trimmed(value, buffer, sizeof(buffer)))
    val = parse_number<int>(buffer);
  if (!val.has_value()) {
    ESP_LOGW(TAG, "Not a whole number: '%s'", value);
    return false;
  }
  out = *val;
  return true;
}

}  // namespace esphome::fendt_caravan
