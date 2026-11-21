from dataclasses import dataclass, field

import esphome.codegen as cg
from esphome.components import modbus
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE
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

# Start addresses of the register blocks; the protocol does not allow one request to cross them
BLOCK_SYSTEM_INFO = 0x000A
BLOCK_RAM = 0x0100
BLOCK_SETTINGS = 0xE002
BLOCK_HISTORY = 0xF000
HISTORY_MAX_DAY = 0x03FF

# Offsets of the registers inside the system information block (0x000A)
REG_SYSTEM_VOLTAGE_AND_CHARGING_CURRENT = 0x00
REG_DISCHARGING_CURRENT_AND_PRODUCT_TYPE = 0x01

# Offsets of the registers inside the live data block (0x0100)
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
# B16 to B31 of the fault information
REG_FAULTS_HIGH = 0x21
# B0 to B15 of the fault information
REG_FAULTS_LOW = 0x22

# Settings are addressed by their absolute address; offset = address - BLOCK_SETTINGS
SETTING_CHARGING_CURRENT_LIMIT = 0xE001
SETTING_SYSTEM_VOLTAGE = 0xE003
SETTING_SPECIAL_POWER_CONTROL = 0xE021

SETTINGS_PUBLISHER = cg.HubFunction(
    "settings",
    cg.void,
    [(cg.std_span.template(cg.uint16.operator("const")), "registers")],
    define="USE_SRNE_ML24XX_SETTINGS_PUBLISHER",
)
SETTINGS = SETTINGS_PUBLISHER.param("registers")


def setting(address: int) -> MockObj:
    """The value of the setting at `address` inside the settings publisher."""
    return SETTINGS[address - BLOCK_SETTINGS]


_request_sensor_slot = cg.slot_counter("SRNE_ML24XX_SENSOR_COUNT")
_request_binary_sensor_slot = cg.slot_counter("SRNE_ML24XX_BINARY_SENSOR_COUNT")
_request_history_day_slot = cg.slot_counter("SRNE_ML24XX_HISTORY_DAY_COUNT")


@dataclass
class _HubData:
    system_info: bool = False
    settings: bool = False
    history_days: set[int] = field(default_factory=set)


def _hub_data(hub: MockObj) -> _HubData:
    return CORE.data.setdefault(DOMAIN, {}).setdefault(str(hub), _HubData())


def request_block(hub: MockObj, block: int) -> None:
    """Make the hub read `block` when one of its entities needs it."""
    data = _hub_data(hub)
    if block == BLOCK_SYSTEM_INFO and not data.system_info:
        data.system_info = True
        cg.add(hub.request_system_info())
    elif block == BLOCK_SETTINGS and not data.settings:
        data.settings = True
        cg.add(hub.request_settings())
    elif block >= BLOCK_HISTORY and (day := block - BLOCK_HISTORY) not in (
        days := data.history_days
    ):
        days.add(day)
        _request_history_day_slot(str(hub))
        cg.add(hub.add_history_day(day))


def register_sensor(hub: MockObj, block: int, var: MockObj) -> cg.Expression:
    request_block(hub, block)
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
