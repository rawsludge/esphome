from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_CONFIG
from esphome.types import ConfigType

from . import (
    BLOCK_SETTINGS,
    CONF_SRNE_ML24XX_ID,
    SETTING_SPECIAL_POWER_CONTROL,
    SETTING_SYSTEM_VOLTAGE,
    SETTINGS_PUBLISHER,
    SRNE_ML24XX_CHILD_SCHEMA,
    request_block,
    setting,
    srne_ml24xx_ns,
)

DEPENDENCIES = ["srne_ml24xx"]

CONF_BATTERY_TYPE = "battery_type"
CONF_CHARGING_METHOD = "charging_method"
CONF_LOAD_WORKING_MODE = "load_working_mode"
CONF_SYSTEM_VOLTAGE = "system_voltage"

SETTING_BATTERY_TYPE = 0xE004
SETTING_LOAD_WORKING_MODE = 0xE01D

# Option index = register value
BATTERY_TYPE_OPTIONS = ["Self-customized", "Open", "Sealed", "Gel", "Lithium"]
LOAD_WORKING_MODE_OPTIONS = [
    "Light control",
    *(f"Light control, off after {hours} h" for hours in range(1, 15)),
    "Manual",
    "Debugging",
    "Normal on",
]
# Bits b0-b1 of the low byte of E021
CHARGING_METHOD_OPTIONS = ["Direct", "PWM"]
# Same order as SYSTEM_VOLTAGES in srne_ml24xx_registers.h
SYSTEM_VOLTAGE_OPTIONS = ["12V", "24V", "36V", "48V", "Auto"]

publish_select_index = srne_ml24xx_ns.publish_select_index
system_voltage_index = srne_ml24xx_ns.system_voltage_index

_SELECT_SCHEMA = select.select_schema(
    select.ApplySelect, entity_category=ENTITY_CATEGORY_CONFIG
)

_KEYS = (
    CONF_BATTERY_TYPE,
    CONF_SYSTEM_VOLTAGE,
    CONF_LOAD_WORKING_MODE,
    CONF_CHARGING_METHOD,
)

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {cv.Optional(key): _SELECT_SCHEMA for key in _KEYS}
    ),
    cv.has_at_least_one_key(*_KEYS),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    selects = select.sub_selects(config, parent=hub)
    # key: (options, write call, option index from the settings block)
    fields = {
        CONF_BATTERY_TYPE: (
            BATTERY_TYPE_OPTIONS,
            f"write_setting({SETTING_BATTERY_TYPE:#06x}, index)",
            setting(SETTING_BATTERY_TYPE),
        ),
        CONF_SYSTEM_VOLTAGE: (
            SYSTEM_VOLTAGE_OPTIONS,
            "write_system_voltage(index)",
            system_voltage_index(setting(SETTING_SYSTEM_VOLTAGE) >> 8),
        ),
        CONF_LOAD_WORKING_MODE: (
            LOAD_WORKING_MODE_OPTIONS,
            f"write_setting({SETTING_LOAD_WORKING_MODE:#06x}, index)",
            setting(SETTING_LOAD_WORKING_MODE),
        ),
        CONF_CHARGING_METHOD: (
            CHARGING_METHOD_OPTIONS,
            f"write_setting_bits({SETTING_SPECIAL_POWER_CONTROL:#06x}, 0x0003, index)",
            setting(SETTING_SPECIAL_POWER_CONTROL) & 0x03,
        ),
    }
    for key, (options, write, index) in fields.items():
        var = await selects(
            key, None, options=options, apply=write, publish="on_success"
        )
        if var is None:
            continue
        request_block(hub, BLOCK_SETTINGS)
        SETTINGS_PUBLISHER.add(hub, publish_select_index(parent_ref(var), index))
