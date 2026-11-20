#ifdef USE_HOST

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <span>

#include "esphome/components/srne_ml24xx/srne_ml24xx.h"
#include "esphome/core/application.h"

namespace esphome::srne_ml24xx::testing {

using RamRegisters = std::array<uint16_t, RAM_REGISTER_COUNT>;

// Sample read of 0x0100-0x0122, taken from the renogy-rover-reader-service reference tests
static constexpr RamRegisters SAMPLE = {
    0x0064, 0x0102, 0x062E, 0x1387, 0x0101, 0x022F, 0x0090, 0x0138, 0x0692, 0x020D, 0x0000, 0x00EE,
    0x0117, 0x07CF, 0x0100, 0x022E, 0x003D, 0x0065, 0x0020, 0x1068, 0x01B9, 0x0200, 0x0009, 0x0080,
    0x20B4, 0x1C00, 0x1ABE, 0x3B00, 0x3ADE, 0x68B1, 0x1B3A, 0x0D49, 0xA102, 0x0000, 0x0000,
};

static constexpr uint8_t REG_FAULTS_HIGH = 0x21;

static float decode(uint8_t reg, RegisterFormat format, std::span<const uint16_t> registers = SAMPLE) {
  return decode_register(registers, reg, format);
}

TEST(SrneMl24xxDecodeTest, DecodesSampleLikeReference) {
  using enum RegisterFormat;
  EXPECT_FLOAT_EQ(decode(0x00, REGISTER_FORMAT_U16), 100.0f);
  EXPECT_FLOAT_EQ(decode(0x01, REGISTER_FORMAT_U16), 258.0f);
  EXPECT_FLOAT_EQ(decode(0x03, REGISTER_FORMAT_TEMPERATURE_HIGH_BYTE), 19.0f);
  EXPECT_FLOAT_EQ(decode(0x03, REGISTER_FORMAT_TEMPERATURE_LOW_BYTE), -7.0f);
  EXPECT_FLOAT_EQ(decode(0x13, REGISTER_FORMAT_U16), 4200.0f);
  EXPECT_FLOAT_EQ(decode(0x18, REGISTER_FORMAT_U32), static_cast<float>(548674560u));
  EXPECT_FLOAT_EQ(decode(0x1A, REGISTER_FORMAT_U32), static_cast<float>(448674560u));
  EXPECT_FLOAT_EQ(decode(0x1C, REGISTER_FORMAT_U32), static_cast<float>(987654321u));
  EXPECT_FLOAT_EQ(decode(0x1E, REGISTER_FORMAT_U32), static_cast<float>(456789321u));
  EXPECT_FLOAT_EQ(decode(REG_LOAD_AND_CHARGING_STATE, REGISTER_FORMAT_HIGH_BYTE_LOW_7_BITS), 33.0f);
}

TEST(SrneMl24xxDecodeTest, TemperatureUsesSignAndMagnitude) {
  EXPECT_EQ(decode_temperature(0x00), 0);
  EXPECT_EQ(decode_temperature(0x7F), 127);
  EXPECT_EQ(decode_temperature(0x80), 0);
  EXPECT_EQ(decode_temperature(0x81), -1);
  EXPECT_EQ(decode_temperature(0xFF), -127);
}

TEST(SrneMl24xxDecodeTest, OutOfRangeRegisterIsNan) {
  const std::array<uint16_t, 2> short_read{0x1234, 0x5678};
  EXPECT_TRUE(std::isnan(decode(2, RegisterFormat::REGISTER_FORMAT_U16, short_read)));
  EXPECT_TRUE(std::isnan(decode(1, RegisterFormat::REGISTER_FORMAT_U32, short_read)));
  EXPECT_FLOAT_EQ(decode(0, RegisterFormat::REGISTER_FORMAT_U32, short_read), static_cast<float>(0x12345678u));
}

class SrneMl24xxTest : public ::testing::Test {
 protected:
  SrneMl24xx hub_;
  SrneMl24xxSensor battery_voltage_{0x01, RegisterFormat::REGISTER_FORMAT_U16, 0.1f};
  SrneMl24xxSensor charging_current_{0x02, RegisterFormat::REGISTER_FORMAT_U16, 0.01f};
  SrneMl24xxSensor battery_temperature_{0x03, RegisterFormat::REGISTER_FORMAT_TEMPERATURE_LOW_BYTE, 1.0f};
  SrneMl24xxSensor total_generation_{0x1C, RegisterFormat::REGISTER_FORMAT_U32, 1.0f};
  SrneMl24xxBinarySensor load_status_{REG_LOAD_AND_CHARGING_STATE, 15};
  SrneMl24xxBinarySensor ambient_too_hot_{REG_FAULTS_HIGH, 6};
  SrneMl24xxBinarySensor counter_current_{REG_FAULTS_HIGH, 10};
  text_sensor::TextSensor charging_state_;

  void SetUp() override {
    new (&App) Application;
    App.pre_setup("srne-ml24xx-tests", 17, "SRNE ML24xx tests", 17);
    this->hub_.register_sensor(&this->battery_voltage_);
    this->hub_.register_sensor(&this->charging_current_);
    this->hub_.register_sensor(&this->battery_temperature_);
    this->hub_.register_sensor(&this->total_generation_);
    this->hub_.register_binary_sensor(&this->load_status_);
    this->hub_.register_binary_sensor(&this->ambient_too_hot_);
    this->hub_.register_binary_sensor(&this->counter_current_);
    this->hub_.set_charging_state_text_sensor(&this->charging_state_);
  }

  void TearDown() override { App.~Application(); }

  void receive(std::span<const uint16_t> registers, uint16_t start = RAM_START_ADDRESS,
               modbus::ResponseStatus status = std::nullopt) {
    this->hub_.on_read_holding_registers(start, registers, status);
  }

  static RamRegisters with(uint8_t reg, uint16_t value) {
    RamRegisters registers = SAMPLE;
    registers[reg] = value;
    return registers;
  }
};

TEST_F(SrneMl24xxTest, PublishesScaledValues) {
  this->receive(SAMPLE);
  EXPECT_NEAR(this->battery_voltage_.get_state(), 25.8f, 1e-4f);
  EXPECT_NEAR(this->charging_current_.get_state(), 15.82f, 1e-4f);
  EXPECT_FLOAT_EQ(this->battery_temperature_.get_state(), -7.0f);
  EXPECT_FLOAT_EQ(this->total_generation_.get_state(), static_cast<float>(987654321u));
  EXPECT_TRUE(this->load_status_.get_state());
  EXPECT_FALSE(this->ambient_too_hot_.get_state());
  EXPECT_FALSE(this->counter_current_.get_state());
  EXPECT_EQ(this->charging_state_.get_state(), "MPPT");
  EXPECT_FALSE(this->hub_.status_has_warning());
}

TEST_F(SrneMl24xxTest, LoadOffKeepsBrightnessAndChargingState) {
  this->receive(with(REG_LOAD_AND_CHARGING_STATE, 0x2105));
  EXPECT_FALSE(this->load_status_.get_state());
  EXPECT_EQ(this->charging_state_.get_state(), "Floating");
}

TEST_F(SrneMl24xxTest, ChargingStateNames) {
  static constexpr std::array<const char *, 8> NAMES = {
      "Deactivated", "Activated", "MPPT", "Equalizing", "Boost", "Floating", "Current limiting", "Unknown",
  };
  for (uint8_t state = 0; state < NAMES.size(); state++) {
    this->receive(with(REG_LOAD_AND_CHARGING_STATE, 0x8000 | state));
    EXPECT_EQ(this->charging_state_.get_state(), NAMES[state]) << "state " << int(state);
  }
  this->receive(with(REG_LOAD_AND_CHARGING_STATE, 0x80FF));
  EXPECT_EQ(this->charging_state_.get_state(), "Unknown");
}

TEST_F(SrneMl24xxTest, FaultBitsComeFromHighWord) {
  this->receive(with(REG_FAULTS_HIGH, 0x0040));
  EXPECT_TRUE(this->ambient_too_hot_.get_state());
  EXPECT_FALSE(this->counter_current_.get_state());

  this->receive(with(REG_FAULTS_HIGH, 0x0400));
  EXPECT_FALSE(this->ambient_too_hot_.get_state());
  EXPECT_TRUE(this->counter_current_.get_state());

  // B0-B15 in 0x0122 are reserved
  this->receive(with(REG_FAULTS_HIGH + 1, 0xFFFF));
  EXPECT_FALSE(this->ambient_too_hot_.get_state());
  EXPECT_FALSE(this->counter_current_.get_state());
}

TEST_F(SrneMl24xxTest, ExceptionResponseSetsWarningWithoutPublishing) {
  this->receive({}, RAM_START_ADDRESS, modbus::ExceptionCode::ILLEGAL_DATA_ADDRESS);
  EXPECT_TRUE(this->hub_.status_has_warning());
  EXPECT_FALSE(this->battery_voltage_.has_state());
  EXPECT_FALSE(this->load_status_.has_state());
  EXPECT_FALSE(this->charging_state_.has_state());

  this->receive(SAMPLE);
  EXPECT_FALSE(this->hub_.status_has_warning());
  EXPECT_TRUE(this->battery_voltage_.has_state());
}

TEST_F(SrneMl24xxTest, UnexpectedBlockIsIgnored) {
  this->receive(std::span<const uint16_t>(SAMPLE).first(RAM_REGISTER_COUNT - 1));
  EXPECT_TRUE(this->hub_.status_has_warning());
  EXPECT_FALSE(this->battery_voltage_.has_state());

  this->receive(SAMPLE, RAM_START_ADDRESS + 1);
  EXPECT_FALSE(this->battery_voltage_.has_state());
}

TEST_F(SrneMl24xxTest, NoResponseSetsWarningAndDoesNotRetry) {
  EXPECT_FALSE(this->hub_.on_no_response(std::span<const uint8_t>{}));
  EXPECT_TRUE(this->hub_.status_has_warning());
}

}  // namespace esphome::srne_ml24xx::testing

#endif  // USE_HOST
