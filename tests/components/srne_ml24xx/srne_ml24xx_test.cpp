#ifdef USE_HOST

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <span>
#include <vector>

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
static constexpr uint8_t REG_FAULTS_LOW = 0x22;
static constexpr uint16_t HISTORY_DAY_3 = HISTORY_START_ADDRESS | 3;

// Exposes the requests the hub queued, in queue order
class ProbeBus : public modbus::ModbusClientHub {
 public:
  std::vector<std::vector<uint8_t>> queued_pdus() const {
    std::vector<std::vector<uint8_t>> pdus;
    for (const auto &cmd : this->tx_buffer_) {
      if (cmd.state != modbus::FrameState::READY)
        continue;
      auto pdu = cmd.frame.pdu();
      pdus.emplace_back(pdu.begin(), pdu.end());
    }
    return pdus;
  }
  /// Start addresses of the queued reads
  std::vector<uint16_t> queued_reads() const {
    std::vector<uint16_t> starts;
    for (const auto &pdu : this->queued_pdus()) {
      if (pdu[0] == 0x03)
        starts.push_back(encode_uint16(pdu[1], pdu[2]));
    }
    return starts;
  }
  void clear() { this->clear_tx_queue_for_address(0x01); }
};

class TestSwitch : public switch_::Switch {
 protected:
  void write_state(bool state) override {}
};

static std::vector<uint16_t> last_settings;  // NOLINT
static void record_settings(std::span<const uint16_t> registers) {
  last_settings.assign(registers.begin(), registers.end());
}

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
  ProbeBus bus_;
  SrneMl24xx hub_;
  SrneMl24xxSensor battery_voltage_{RAM_START_ADDRESS, 0x01, RegisterFormat::REGISTER_FORMAT_U16, 0.1f};
  SrneMl24xxSensor charging_current_{RAM_START_ADDRESS, 0x02, RegisterFormat::REGISTER_FORMAT_U16, 0.01f};
  SrneMl24xxSensor battery_temperature_{RAM_START_ADDRESS, 0x03, RegisterFormat::REGISTER_FORMAT_TEMPERATURE_LOW_BYTE,
                                        1.0f};
  SrneMl24xxSensor total_generation_{RAM_START_ADDRESS, 0x1C, RegisterFormat::REGISTER_FORMAT_U32, 1.0f};
  SrneMl24xxSensor max_system_voltage_{SYSTEM_INFO_START_ADDRESS, 0x00, RegisterFormat::REGISTER_FORMAT_SYSTEM_VOLTAGE,
                                       1.0f};
  SrneMl24xxSensor rated_charging_current_{SYSTEM_INFO_START_ADDRESS, 0x00, RegisterFormat::REGISTER_FORMAT_LOW_BYTE,
                                           1.0f};
  SrneMl24xxSensor day_3_generation_{HISTORY_DAY_3, 0x08, RegisterFormat::REGISTER_FORMAT_U16, 1.0f};
  SrneMl24xxBinarySensor load_status_{REG_LOAD_AND_CHARGING_STATE, 15};
  SrneMl24xxBinarySensor no_battery_{REG_FAULTS_HIGH, 7};
  SrneMl24xxBinarySensor controller_too_hot_{REG_FAULTS_LOW, 5};
  SrneMl24xxBinarySensor mains_power_{REG_FAULTS_HIGH, 6};
  text_sensor::TextSensor charging_state_;
  text_sensor::TextSensor product_model_;
  text_sensor::TextSensor product_type_;
  text_sensor::TextSensor software_version_;
  text_sensor::TextSensor hardware_version_;
  text_sensor::TextSensor serial_number_;
  TestSwitch load_;

  void SetUp() override {
    new (&App) Application;
    App.pre_setup("srne-ml24xx-tests", 17, "SRNE ML24xx tests", 17);
    this->hub_.register_sensor(&this->battery_voltage_);
    this->hub_.register_sensor(&this->charging_current_);
    this->hub_.register_sensor(&this->battery_temperature_);
    this->hub_.register_sensor(&this->total_generation_);
    this->hub_.register_sensor(&this->max_system_voltage_);
    this->hub_.register_sensor(&this->rated_charging_current_);
    this->hub_.register_sensor(&this->day_3_generation_);
    this->hub_.register_binary_sensor(&this->load_status_);
    this->hub_.register_binary_sensor(&this->no_battery_);
    this->hub_.register_binary_sensor(&this->controller_too_hot_);
    this->hub_.register_binary_sensor(&this->mains_power_);
    this->hub_.set_charging_state_text_sensor(&this->charging_state_);
    this->hub_.set_product_model_text_sensor(&this->product_model_);
    this->hub_.set_product_type_text_sensor(&this->product_type_);
    this->hub_.set_software_version_text_sensor(&this->software_version_);
    this->hub_.set_hardware_version_text_sensor(&this->hardware_version_);
    this->hub_.set_serial_number_text_sensor(&this->serial_number_);
    this->hub_.set_load_switch(&this->load_);
    this->hub_.set_parent(&this->bus_);
    this->hub_.set_address(0x01);
    last_settings.clear();
  }

  /// Settings E002-E021 with E003 (system voltage) and E021 (special power control) set
  void receive_settings(uint16_t system_voltage, uint16_t special_power_control) {
    std::array<uint16_t, SETTINGS_REGISTER_COUNT> settings{};
    settings[SETTING_SYSTEM_VOLTAGE - SETTINGS_START_ADDRESS] = system_voltage;
    settings[SETTING_SPECIAL_POWER_CONTROL - SETTINGS_START_ADDRESS] = special_power_control;
    this->receive(settings, SETTINGS_START_ADDRESS);
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
  EXPECT_FALSE(this->no_battery_.get_state());
  EXPECT_FALSE(this->controller_too_hot_.get_state());
  EXPECT_EQ(this->charging_state_.get_state(), "MPPT");
  EXPECT_FALSE(this->load_.state);
  EXPECT_FALSE(this->hub_.status_has_warning());
  // Sensors of other blocks are left alone
  EXPECT_FALSE(this->max_system_voltage_.has_state());
  EXPECT_FALSE(this->day_3_generation_.has_state());
}

TEST_F(SrneMl24xxTest, LoadSwitchFollowsLoadCommand) {
  this->receive(with(REG_LOAD_COMMAND, 1));
  EXPECT_TRUE(this->load_.state);
  this->receive(with(REG_LOAD_COMMAND, 0));
  EXPECT_FALSE(this->load_.state);
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

TEST_F(SrneMl24xxTest, FaultBitsUseBothWords) {
  // B23 (no battery) is bit 7 of 0x0121
  this->receive(with(REG_FAULTS_HIGH, 0x0080));
  EXPECT_TRUE(this->no_battery_.get_state());
  EXPECT_FALSE(this->mains_power_.get_state());
  EXPECT_FALSE(this->controller_too_hot_.get_state());

  // B22 (mains power supply) is bit 6 of 0x0121
  this->receive(with(REG_FAULTS_HIGH, 0x0040));
  EXPECT_FALSE(this->no_battery_.get_state());
  EXPECT_TRUE(this->mains_power_.get_state());

  // B5 (controller temperature too high) is bit 5 of 0x0122
  this->receive(with(REG_FAULTS_LOW, 0x0020));
  EXPECT_FALSE(this->no_battery_.get_state());
  EXPECT_TRUE(this->controller_too_hot_.get_state());
}

TEST_F(SrneMl24xxTest, DecodesSystemInformation) {
  // Examples from the protocol: 24 V / 30 A, a controller rated for 20 A discharge, model MT4830,
  // software V03.02.01, hardware V01.02.03, serial number 0F01FFFF
  const std::array<uint16_t, SYSTEM_INFO_REGISTER_COUNT> info = {
      0x181E, 0x1400, 0x2020, 0x2020, 0x4D54, 0x3438, 0x3330, 0x2020, 0x2020,
      0x2020, 0x0003, 0x0201, 0x0001, 0x0203, 0x0F01, 0xFFFF, 0x0001,
  };
  this->receive(info, SYSTEM_INFO_START_ADDRESS);
  EXPECT_FLOAT_EQ(this->max_system_voltage_.get_state(), 24.0f);
  EXPECT_FLOAT_EQ(this->rated_charging_current_.get_state(), 30.0f);
  EXPECT_EQ(this->product_model_.get_state(), "MT4830");
  EXPECT_EQ(this->product_type_.get_state(), "Controller");
  EXPECT_EQ(this->software_version_.get_state(), "V03.02.01");
  EXPECT_EQ(this->hardware_version_.get_state(), "V01.02.03");
  EXPECT_EQ(this->serial_number_.get_state(), "0F01FFFF");
  // RAM sensors are left alone
  EXPECT_FALSE(this->battery_voltage_.has_state());
}

TEST_F(SrneMl24xxTest, AutomaticSystemVoltageIsNotPublished) {
  std::array<uint16_t, SYSTEM_INFO_REGISTER_COUNT> info{};
  info[0] = 0xFF3C;
  this->receive(info, SYSTEM_INFO_START_ADDRESS);
  EXPECT_FALSE(this->max_system_voltage_.has_state());
  EXPECT_FLOAT_EQ(this->rated_charging_current_.get_state(), 60.0f);
}

TEST_F(SrneMl24xxTest, UpdateReadsSystemInformationUntilReceived) {
  this->hub_.request_system_info();
  this->hub_.update();
  EXPECT_EQ(this->bus_.queued_reads(), (std::vector<uint16_t>{RAM_START_ADDRESS, SYSTEM_INFO_START_ADDRESS}));

  this->bus_.clear();
  this->hub_.update();
  EXPECT_EQ(this->bus_.queued_reads(), (std::vector<uint16_t>{RAM_START_ADDRESS, SYSTEM_INFO_START_ADDRESS}));

  this->bus_.clear();
  this->receive(std::array<uint16_t, SYSTEM_INFO_REGISTER_COUNT>{}, SYSTEM_INFO_START_ADDRESS);
  this->hub_.update();
  EXPECT_EQ(this->bus_.queued_reads(), (std::vector<uint16_t>{RAM_START_ADDRESS}));
}

TEST_F(SrneMl24xxTest, UpdateReadsSettingsAndHistoryDays) {
  this->hub_.request_settings();
  this->hub_.add_history_day(1);
  this->hub_.add_history_day(3);
  this->hub_.update();
  EXPECT_EQ(this->bus_.queued_reads(),
            (std::vector<uint16_t>{RAM_START_ADDRESS, SETTINGS_START_ADDRESS, 0xF001, HISTORY_DAY_3}));
  for (const auto &pdu : this->bus_.queued_pdus()) {
    const uint16_t start = encode_uint16(pdu[1], pdu[2]);
    const uint16_t count = encode_uint16(pdu[3], pdu[4]);
    if (start == SETTINGS_START_ADDRESS)
      EXPECT_EQ(count, SETTINGS_REGISTER_COUNT);
    else if (start != RAM_START_ADDRESS)
      EXPECT_EQ(count, HISTORY_REGISTER_COUNT);
  }
}

TEST_F(SrneMl24xxTest, HistorySensorsOnlyTakeTheirDay) {
  std::array<uint16_t, HISTORY_REGISTER_COUNT> day{};
  day[8] = 990;
  this->receive(day, HISTORY_START_ADDRESS | 2);
  EXPECT_FALSE(this->day_3_generation_.has_state());
  EXPECT_FALSE(this->hub_.status_has_warning());

  this->receive(day, HISTORY_DAY_3);
  EXPECT_FLOAT_EQ(this->day_3_generation_.get_state(), 990.0f);

  // A history read of the wrong length is rejected
  this->receive(std::span<const uint16_t>(day).first(9), HISTORY_DAY_3);
  EXPECT_TRUE(this->hub_.status_has_warning());
}

TEST_F(SrneMl24xxTest, SettingsGoToThePublisher) {
  this->hub_.set_settings_publisher(record_settings);
  this->receive_settings(0x180C, 0x0001);
  ASSERT_EQ(last_settings.size(), SETTINGS_REGISTER_COUNT);
  EXPECT_EQ(last_settings[SETTING_SYSTEM_VOLTAGE - SETTINGS_START_ADDRESS], 0x180C);
}

TEST_F(SrneMl24xxTest, PartialWritesNeedTheSettingsFirst) {
  EXPECT_FALSE(this->hub_.write_setting_bits(SETTING_SPECIAL_POWER_CONTROL, 0x0100, 0x0100));
  EXPECT_FALSE(this->hub_.write_system_voltage(1));
  EXPECT_TRUE(this->bus_.queued_pdus().empty());
}

TEST_F(SrneMl24xxTest, PartialWritesKeepTheOtherBits) {
  this->receive_settings(0x0C0C, 0x0201);

  // Clearing special power control (b9 of the high byte) keeps PWM charging (b0)
  ASSERT_TRUE(this->hub_.write_setting_bits(SETTING_SPECIAL_POWER_CONTROL, 0x0200, 0));
  // A second change before the next read builds on the first one
  ASSERT_TRUE(this->hub_.write_setting_bits(SETTING_SPECIAL_POWER_CONTROL, 0x0003, 0));
  // 24 V keeps the recognized voltage in the low byte
  ASSERT_TRUE(this->hub_.write_system_voltage(1));
  EXPECT_FALSE(this->hub_.write_system_voltage(SYSTEM_VOLTAGE_COUNT));
  // Settings without other values in their register are refused
  EXPECT_FALSE(this->hub_.write_setting_bits(0xE004, 0x00FF, 1));

  EXPECT_EQ(this->bus_.queued_pdus(), (std::vector<std::vector<uint8_t>>{
                                          {0x06, 0xE0, 0x21, 0x00, 0x01},
                                          {0x06, 0xE0, 0x21, 0x00, 0x00},
                                          {0x06, 0xE0, 0x03, 0x18, 0x0C},
                                      }));
}

TEST_F(SrneMl24xxTest, WritesQueueTheExpectedRequests) {
  ASSERT_TRUE(this->hub_.write_scaled_setting(0xE005, 14.4f, 10.0f));
  ASSERT_TRUE(this->hub_.write_scaled_setting(0xE001, 20.0f, 100.0f));
  ASSERT_TRUE(this->hub_.set_load(true));
  ASSERT_TRUE(this->hub_.factory_reset());
  ASSERT_TRUE(this->hub_.clear_history());
  // Examples 4.18, 4.23, 4.16, 4.21 and 4.22 of the protocol
  EXPECT_EQ(this->bus_.queued_pdus(), (std::vector<std::vector<uint8_t>>{
                                          {0x06, 0xE0, 0x05, 0x00, 0x90},
                                          {0x06, 0xE0, 0x01, 0x07, 0xD0},
                                          {0x06, 0x01, 0x0A, 0x00, 0x01},
                                          {0x78, 0x00, 0x00, 0x00, 0x01},
                                          {0x79, 0x00, 0x00, 0x00, 0x01},
                                      }));
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
