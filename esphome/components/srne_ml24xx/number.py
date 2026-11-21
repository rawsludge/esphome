from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import number
from esphome.components.const import UNIT_AMPERE_HOUR
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_CONFIG,
    UNIT_AMPERE,
    UNIT_MINUTE,
    UNIT_SECOND,
    UNIT_VOLT,
)
from esphome.types import ConfigType

from . import (
    BLOCK_SETTINGS,
    CONF_SRNE_ML24XX_ID,
    SETTING_CHARGING_CURRENT_LIMIT,
    SETTINGS_PUBLISHER,
    SRNE_ML24XX_CHILD_SCHEMA,
    request_block,
    setting,
)

DEPENDENCIES = ["srne_ml24xx"]

UNIT_DAY = "d"
UNIT_TEMPERATURE_COMPENSATION = "mV/°C/2V"


def _voltage(address: int) -> tuple:
    # The controller takes these for a 12 V system and scales them for higher system voltages
    return (address, 10, 7.0, 17.0, 0.1, UNIT_VOLT, DEVICE_CLASS_VOLTAGE)


def _duration(address: int, unit: str, low: int, high: int, step: int = 1) -> tuple:
    return (address, 1, low, high, step, unit, DEVICE_CLASS_DURATION)


# key: (address, raw value per unit, min, max, step, unit, device class)
_FIELDS = {
    # Write only; supported by some models
    "charging_current_limit": (
        SETTING_CHARGING_CURRENT_LIMIT,
        100,
        0.0,
        100.0,
        0.01,
        UNIT_AMPERE,
        DEVICE_CLASS_CURRENT,
    ),
    "nominal_battery_capacity": (
        0xE002,
        1,
        1,
        65535,
        1,
        UNIT_AMPERE_HOUR,
        cv.UNDEFINED,
    ),
    "over_voltage_threshold": _voltage(0xE005),
    "charging_limit_voltage": _voltage(0xE006),
    "equalizing_charging_voltage": _voltage(0xE007),
    "boost_charging_voltage": _voltage(0xE008),
    "floating_charging_voltage": _voltage(0xE009),
    "boost_charging_recovery_voltage": _voltage(0xE00A),
    "over_discharge_recovery_voltage": _voltage(0xE00B),
    "under_voltage_warning_level": _voltage(0xE00C),
    "over_discharge_voltage": _voltage(0xE00D),
    "discharging_limit_voltage": _voltage(0xE00E),
    "over_discharge_time_delay": _duration(0xE010, UNIT_SECOND, 0, 120),
    "equalizing_charging_time": _duration(0xE011, UNIT_MINUTE, 0, 300, 10),
    "boost_charging_time": _duration(0xE012, UNIT_MINUTE, 10, 300, 10),
    # 0 turns equalizing off
    "equalizing_charging_interval": _duration(0xE013, UNIT_DAY, 0, 255, 5),
    # 0 turns temperature compensation off
    "temperature_compensation_factor": (
        0xE014,
        1,
        0,
        5,
        1,
        UNIT_TEMPERATURE_COMPENSATION,
        cv.UNDEFINED,
    ),
    "light_control_delay": _duration(0xE01E, UNIT_MINUTE, 0, 60),
    "light_control_voltage": (0xE01F, 1, 1, 40, 1, UNIT_VOLT, DEVICE_CLASS_VOLTAGE),
}

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            cv.Optional(key): number.number_schema(
                number.ApplyNumber,
                entity_category=ENTITY_CATEGORY_CONFIG,
                unit_of_measurement=unit,
                device_class=device_class,
            )
            for key, (*_, unit, device_class) in _FIELDS.items()
        }
    ),
    cv.has_at_least_one_key(*_FIELDS),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    numbers = number.sub_numbers(config, parent=hub)
    for key, (address, raw_per_unit, low, high, step, *_) in _FIELDS.items():
        var = await numbers(
            key,
            None,
            min_value=low,
            max_value=high,
            step=step,
            apply=f"write_scaled_setting({address:#06x}, value, {raw_per_unit}.0f)",
            publish="on_success",
        )
        if var is None or address == SETTING_CHARGING_CURRENT_LIMIT:
            continue
        request_block(hub, BLOCK_SETTINGS)
        value = setting(address)
        if raw_per_unit != 1:
            value = value * (1 / raw_per_unit)
        SETTINGS_PUBLISHER.add(hub, parent_ref(var).publish_state(value))
