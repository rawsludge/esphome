from functools import partial

import esphome.codegen as cg
from esphome.components import sensor
from esphome.components.const import UNIT_AMPERE_HOUR
import esphome.config_validation as cv
from esphome.const import (
    CONF_DAY,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    ENTITY_CATEGORY_DIAGNOSTIC,
    ICON_BRIGHTNESS_5,
    ICON_COUNTER,
    ICON_TIMER,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    UNIT_VOLT,
    UNIT_WATT,
    UNIT_WATT_HOURS,
)
from esphome.types import ConfigType

from . import (
    BLOCK_HISTORY,
    BLOCK_RAM,
    BLOCK_SETTINGS,
    BLOCK_SYSTEM_INFO,
    CONF_SRNE_ML24XX_ID,
    HISTORY_MAX_DAY,
    REG_BATTERY_SOC,
    REG_BATTERY_VOLTAGE,
    REG_CHARGING_CURRENT,
    REG_DAILY_BATTERY_VOLTAGE_MAX,
    REG_DAILY_BATTERY_VOLTAGE_MIN,
    REG_DAILY_CHARGING_AMP_HOURS,
    REG_DAILY_CHARGING_CURRENT_MAX,
    REG_DAILY_CHARGING_POWER_MAX,
    REG_DAILY_DISCHARGING_AMP_HOURS,
    REG_DAILY_DISCHARGING_CURRENT_MAX,
    REG_DAILY_DISCHARGING_POWER_MAX,
    REG_DAILY_POWER_CONSUMPTION,
    REG_DAILY_POWER_GENERATION,
    REG_DISCHARGING_CURRENT_AND_PRODUCT_TYPE,
    REG_LOAD_AND_CHARGING_STATE,
    REG_LOAD_CURRENT,
    REG_LOAD_POWER,
    REG_LOAD_VOLTAGE,
    REG_SOLAR_CURRENT,
    REG_SOLAR_POWER,
    REG_SOLAR_VOLTAGE,
    REG_SYSTEM_VOLTAGE_AND_CHARGING_CURRENT,
    REG_TEMPERATURES,
    REG_TOTAL_CHARGING_AMP_HOURS,
    REG_TOTAL_DISCHARGING_AMP_HOURS,
    REG_TOTAL_FULL_CHARGES,
    REG_TOTAL_OPERATING_DAYS,
    REG_TOTAL_OVER_DISCHARGES,
    REG_TOTAL_POWER_CONSUMPTION,
    REG_TOTAL_POWER_GENERATION,
    SETTING_SYSTEM_VOLTAGE,
    SRNE_ML24XX_CHILD_SCHEMA,
    register_sensor,
    srne_ml24xx_ns,
)

DEPENDENCIES = ["srne_ml24xx"]

CONF_HISTORY = "history"

UNIT_DAY = "d"

SrneMl24xxSensor = srne_ml24xx_ns.class_("SrneMl24xxSensor", sensor.Sensor)
RegisterFormat = srne_ml24xx_ns.enum("RegisterFormat", is_class=True)
U16 = RegisterFormat.REGISTER_FORMAT_U16
U32 = RegisterFormat.REGISTER_FORMAT_U32
TEMP_HIGH = RegisterFormat.REGISTER_FORMAT_TEMPERATURE_HIGH_BYTE
TEMP_LOW = RegisterFormat.REGISTER_FORMAT_TEMPERATURE_LOW_BYTE
HIGH_BYTE_LOW_7_BITS = RegisterFormat.REGISTER_FORMAT_HIGH_BYTE_LOW_7_BITS
HIGH_BYTE = RegisterFormat.REGISTER_FORMAT_HIGH_BYTE
LOW_BYTE = RegisterFormat.REGISTER_FORMAT_LOW_BYTE
SYSTEM_VOLTAGE = RegisterFormat.REGISTER_FORMAT_SYSTEM_VOLTAGE


def _voltage(reg: int) -> tuple:
    return (reg, U16, 0.1, UNIT_VOLT, 1, DEVICE_CLASS_VOLTAGE, STATE_CLASS_MEASUREMENT)


def _current(reg: int) -> tuple:
    return (
        reg,
        U16,
        0.01,
        UNIT_AMPERE,
        2,
        DEVICE_CLASS_CURRENT,
        STATE_CLASS_MEASUREMENT,
    )


def _power(reg: int) -> tuple:
    return (reg, U16, 1.0, UNIT_WATT, 0, DEVICE_CLASS_POWER, STATE_CLASS_MEASUREMENT)


def _temperature(fmt: cg.MockObj) -> tuple:
    return (
        REG_TEMPERATURES,
        fmt,
        1.0,
        UNIT_CELSIUS,
        0,
        DEVICE_CLASS_TEMPERATURE,
        STATE_CLASS_MEASUREMENT,
    )


def _amp_hours(reg: int, fmt: cg.MockObj = U16) -> tuple:
    return (
        reg,
        fmt,
        1.0,
        UNIT_AMPERE_HOUR,
        0,
        cv.UNDEFINED,
        STATE_CLASS_TOTAL_INCREASING,
    )


def _energy(reg: int, fmt: cg.MockObj = U16) -> tuple:
    return (
        reg,
        fmt,
        1.0,
        UNIT_WATT_HOURS,
        0,
        DEVICE_CLASS_ENERGY,
        STATE_CLASS_TOTAL_INCREASING,
    )


def _counter(reg: int, unit: str = cv.UNDEFINED) -> tuple:
    return (reg, U16, 1.0, unit, 0, cv.UNDEFINED, STATE_CLASS_TOTAL_INCREASING)


def _rating(reg: int, fmt: cg.MockObj, unit: str, device_class: str) -> tuple:
    return (reg, fmt, 1.0, unit, 0, device_class, cv.UNDEFINED)


# The fields of one day, at the same offsets in the history block of any day
_DAY_FIELDS = {
    "battery_voltage_min": _voltage(REG_DAILY_BATTERY_VOLTAGE_MIN),
    "battery_voltage_max": _voltage(REG_DAILY_BATTERY_VOLTAGE_MAX),
    "charging_current_max": _current(REG_DAILY_CHARGING_CURRENT_MAX),
    "discharging_current_max": _current(REG_DAILY_DISCHARGING_CURRENT_MAX),
    "charging_power_max": _power(REG_DAILY_CHARGING_POWER_MAX),
    "discharging_power_max": _power(REG_DAILY_DISCHARGING_POWER_MAX),
    "charging_amp_hours": _amp_hours(REG_DAILY_CHARGING_AMP_HOURS),
    "discharging_amp_hours": _amp_hours(REG_DAILY_DISCHARGING_AMP_HOURS),
    "power_generation": _energy(REG_DAILY_POWER_GENERATION),
    "power_consumption": _energy(REG_DAILY_POWER_CONSUMPTION),
}

# key: (block, (register offset, format, scale, unit, accuracy, device class, state class))
_FIELDS = {
    "battery_level": (
        BLOCK_RAM,
        (
            REG_BATTERY_SOC,
            U16,
            1.0,
            UNIT_PERCENT,
            0,
            DEVICE_CLASS_BATTERY,
            STATE_CLASS_MEASUREMENT,
        ),
    ),
    "battery_voltage": (BLOCK_RAM, _voltage(REG_BATTERY_VOLTAGE)),
    "battery_charging_current": (BLOCK_RAM, _current(REG_CHARGING_CURRENT)),
    "controller_temperature": (BLOCK_RAM, _temperature(TEMP_HIGH)),
    "battery_temperature": (BLOCK_RAM, _temperature(TEMP_LOW)),
    "load_voltage": (BLOCK_RAM, _voltage(REG_LOAD_VOLTAGE)),
    "load_current": (BLOCK_RAM, _current(REG_LOAD_CURRENT)),
    "load_power": (BLOCK_RAM, _power(REG_LOAD_POWER)),
    "load_brightness": (
        BLOCK_RAM,
        (
            REG_LOAD_AND_CHARGING_STATE,
            HIGH_BYTE_LOW_7_BITS,
            1.0,
            UNIT_PERCENT,
            0,
            cv.UNDEFINED,
            STATE_CLASS_MEASUREMENT,
        ),
    ),
    "solar_panel_voltage": (BLOCK_RAM, _voltage(REG_SOLAR_VOLTAGE)),
    "solar_panel_current": (BLOCK_RAM, _current(REG_SOLAR_CURRENT)),
    "solar_panel_power": (BLOCK_RAM, _power(REG_SOLAR_POWER)),
    **{f"daily_{key}": (BLOCK_RAM, field) for key, field in _DAY_FIELDS.items()},
    "total_operating_days": (BLOCK_RAM, _counter(REG_TOTAL_OPERATING_DAYS, UNIT_DAY)),
    "total_battery_over_discharges": (BLOCK_RAM, _counter(REG_TOTAL_OVER_DISCHARGES)),
    "total_battery_full_charges": (BLOCK_RAM, _counter(REG_TOTAL_FULL_CHARGES)),
    "total_charging_amp_hours": (
        BLOCK_RAM,
        _amp_hours(REG_TOTAL_CHARGING_AMP_HOURS, U32),
    ),
    "total_discharging_amp_hours": (
        BLOCK_RAM,
        _amp_hours(REG_TOTAL_DISCHARGING_AMP_HOURS, U32),
    ),
    "total_power_generation": (BLOCK_RAM, _energy(REG_TOTAL_POWER_GENERATION, U32)),
    "total_power_consumption": (
        BLOCK_RAM,
        _energy(REG_TOTAL_POWER_CONSUMPTION, U32),
    ),
    "max_system_voltage": (
        BLOCK_SYSTEM_INFO,
        _rating(
            REG_SYSTEM_VOLTAGE_AND_CHARGING_CURRENT,
            SYSTEM_VOLTAGE,
            UNIT_VOLT,
            DEVICE_CLASS_VOLTAGE,
        ),
    ),
    "rated_charging_current": (
        BLOCK_SYSTEM_INFO,
        _rating(
            REG_SYSTEM_VOLTAGE_AND_CHARGING_CURRENT,
            LOW_BYTE,
            UNIT_AMPERE,
            DEVICE_CLASS_CURRENT,
        ),
    ),
    "rated_discharging_current": (
        BLOCK_SYSTEM_INFO,
        _rating(
            REG_DISCHARGING_CURRENT_AND_PRODUCT_TYPE,
            HIGH_BYTE,
            UNIT_AMPERE,
            DEVICE_CLASS_CURRENT,
        ),
    ),
    "recognized_system_voltage": (
        BLOCK_SETTINGS,
        _rating(
            SETTING_SYSTEM_VOLTAGE - BLOCK_SETTINGS,
            LOW_BYTE,
            UNIT_VOLT,
            DEVICE_CLASS_VOLTAGE,
        ),
    ),
}

# The history of a past day no longer grows, so its totals are not total_increasing
_HISTORY_FIELDS = {
    key: (
        (reg - REG_DAILY_BATTERY_VOLTAGE_MIN, *rest, STATE_CLASS_TOTAL)
        if state_class == STATE_CLASS_TOTAL_INCREASING
        else (reg - REG_DAILY_BATTERY_VOLTAGE_MIN, *rest, state_class)
    )
    for key, (reg, *rest, state_class) in _DAY_FIELDS.items()
}

_ICONS = {
    "load_brightness": ICON_BRIGHTNESS_5,
    "total_operating_days": ICON_TIMER,
    "total_battery_over_discharges": ICON_COUNTER,
    "total_battery_full_charges": ICON_COUNTER,
}

_DIAGNOSTIC = {
    "controller_temperature",
    "total_operating_days",
    "total_battery_over_discharges",
    "total_battery_full_charges",
    "max_system_voltage",
    "rated_charging_current",
    "rated_discharging_current",
    "recognized_system_voltage",
}


def _sensor_schema(key: str, field: tuple) -> cv.Schema:
    _, _, _, unit, accuracy, device_class, state_class = field
    return sensor.sensor_schema(
        SrneMl24xxSensor,
        unit_of_measurement=unit,
        accuracy_decimals=accuracy,
        device_class=device_class,
        state_class=state_class,
        icon=_ICONS.get(key, cv.UNDEFINED),
        entity_category=(
            ENTITY_CATEGORY_DIAGNOSTIC if key in _DIAGNOSTIC else cv.UNDEFINED
        ),
    )


def _unique_days(days: list[ConfigType]) -> list[ConfigType]:
    seen = set()
    for day in days:
        if day[CONF_DAY] in seen:
            raise cv.Invalid(f"Day {day[CONF_DAY]} is listed more than once")
        seen.add(day[CONF_DAY])
    return days


HISTORY_DAY_SCHEMA = cv.All(
    cv.Schema(
        {
            # Day 0 is today, which the daily_* sensors already cover
            cv.Required(CONF_DAY): cv.int_range(min=1, max=HISTORY_MAX_DAY),
            **{
                cv.Optional(key): _sensor_schema(key, field)
                for key, field in _HISTORY_FIELDS.items()
            },
        }
    ),
    cv.has_at_least_one_key(*_HISTORY_FIELDS),
)

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            **{
                cv.Optional(key): _sensor_schema(key, field)
                for key, (_, field) in _FIELDS.items()
            },
            cv.Optional(CONF_HISTORY): cv.All(
                cv.ensure_list(HISTORY_DAY_SCHEMA), cv.Length(min=1), _unique_days
            ),
        }
    ),
    cv.has_at_least_one_key(*_FIELDS, CONF_HISTORY),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    sensors = sensor.sub_sensors(config)
    for key, (block, (reg, fmt, scale, *_)) in _FIELDS.items():
        await sensors(key, partial(register_sensor, hub, block), block, reg, fmt, scale)
    for day_config in config.get(CONF_HISTORY, []):
        block = BLOCK_HISTORY | day_config[CONF_DAY]
        day_sensors = sensor.sub_sensors(day_config)
        for key, (reg, fmt, scale, *_) in _HISTORY_FIELDS.items():
            await day_sensors(
                key, partial(register_sensor, hub, block), block, reg, fmt, scale
            )
