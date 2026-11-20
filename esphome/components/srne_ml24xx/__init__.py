import esphome.codegen as cg
from esphome.components import modbus
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.cpp_generator import MockObj
from esphome.types import ConfigType

AUTO_LOAD = ["modbus"]
CODEOWNERS = ["@rawsludge"]
DOMAIN = "srne_ml24xx"
MULTI_CONF = True

CONF_SRNE_ML24XX_ID = "srne_ml24xx_id"

srne_ml24xx_ns = cg.esphome_ns.namespace("srne_ml24xx")
SrneMl24xx = srne_ml24xx_ns.class_(
    "SrneMl24xx", cg.PollingComponent, modbus.ModbusClientDevice
)

SRNE_ML24XX_CHILD_SCHEMA = cv.Schema(
    {cv.GenerateID(CONF_SRNE_ML24XX_ID): cv.use_id(SrneMl24xx)}
)

# Offsets of the registers inside the block read from 0x0100
REG_BATTERY_SOC = 0x00
REG_BATTERY_VOLTAGE = 0x01
REG_CHARGING_CURRENT = 0x02
REG_TEMPERATURES = 0x03
REG_LOAD_VOLTAGE = 0x04
REG_LOAD_CURRENT = 0x05
REG_LOAD_POWER = 0x06
REG_SOLAR_VOLTAGE = 0x07
REG_SOLAR_CURRENT = 0x08
REG_SOLAR_POWER = 0x09
REG_DAILY_BATTERY_VOLTAGE_MIN = 0x0B
REG_DAILY_BATTERY_VOLTAGE_MAX = 0x0C
REG_DAILY_CHARGING_CURRENT_MAX = 0x0D
REG_DAILY_DISCHARGING_CURRENT_MAX = 0x0E
REG_DAILY_CHARGING_POWER_MAX = 0x0F
REG_DAILY_DISCHARGING_POWER_MAX = 0x10
REG_DAILY_CHARGING_AMP_HOURS = 0x11
REG_DAILY_DISCHARGING_AMP_HOURS = 0x12
REG_DAILY_POWER_GENERATION = 0x13
REG_DAILY_POWER_CONSUMPTION = 0x14
REG_TOTAL_OPERATING_DAYS = 0x15
REG_TOTAL_OVER_DISCHARGES = 0x16
REG_TOTAL_FULL_CHARGES = 0x17
REG_TOTAL_CHARGING_AMP_HOURS = 0x18
REG_TOTAL_DISCHARGING_AMP_HOURS = 0x1A
REG_TOTAL_POWER_GENERATION = 0x1C
REG_TOTAL_POWER_CONSUMPTION = 0x1E
REG_LOAD_AND_CHARGING_STATE = 0x20
REG_FAULTS_HIGH = 0x21

_request_sensor_slot = cg.slot_counter("SRNE_ML24XX_SENSOR_COUNT")
_request_binary_sensor_slot = cg.slot_counter("SRNE_ML24XX_BINARY_SENSOR_COUNT")


def register_sensor(hub: MockObj, var: MockObj) -> cg.Expression:
    _request_sensor_slot(str(hub))
    return hub.register_sensor(var)


def register_binary_sensor(hub: MockObj, var: MockObj) -> cg.Expression:
    _request_binary_sensor_slot(str(hub))
    return hub.register_binary_sensor(var)


CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(SrneMl24xx)})
    .extend(cv.polling_component_schema("10s"))
    .extend(modbus.modbus_device_schema(0x01))
)


def _final_validate(config: ConfigType) -> None:
    modbus.final_validate_modbus_device(DOMAIN, role="client")(config)


FINAL_VALIDATE_SCHEMA = _final_validate


async def to_code(config: ConfigType) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await modbus.register_modbus_client_device(var, config)
