from esphome import final_validate as fv
import esphome.codegen as cg
from esphome.components import sensor
from esphome.components.const import UNIT_AMPERE_HOUR
import esphome.config_validation as cv
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_BATTERY_VOLTAGE,
    CONF_TYPE,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_DURATION,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_AMPERE,
    UNIT_CELSIUS,
    UNIT_HOUR,
    UNIT_PERCENT,
    UNIT_VOLT,
)
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, DOMAIN, BatteryOneHub, MainControlUnitHub

CONF_TEMP_IN = "temp_in"
CONF_TEMP_OUT = "temp_out"
CONF_WATER_LEVEL = "water_level"
CONF_BATTERY_CURRENT = "battery_current"
CONF_BATTERY_REMAINING_TIME = "battery_remaining_time"
CONF_BATTERY_TEMPERATURE = "battery_temperature"
CONF_BATTERY_CAPACITY = "battery_capacity"


def _sensor_schema(
    hub: MockObjClass = MainControlUnitHub,
    unit_of_measurement: str = cv.UNDEFINED,
    accuracy_decimals: int = cv.UNDEFINED,
    device_class: str = cv.UNDEFINED,
    state_class: str = cv.UNDEFINED,
    icon: str = cv.UNDEFINED,
) -> cv.Schema:
    return sensor.sensor_schema(
        unit_of_measurement=unit_of_measurement,
        accuracy_decimals=accuracy_decimals,
        device_class=device_class,
        state_class=state_class,
        icon=icon,
    ).extend(
        {
            cv.Required(CONF_PARENT_ID): cv.use_id(hub),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_TEMP_IN: _sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
        CONF_TEMP_OUT: _sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            state_class=STATE_CLASS_MEASUREMENT,
            device_class=DEVICE_CLASS_TEMPERATURE,
        ),
        CONF_WATER_LEVEL: _sensor_schema(
            icon="mdi:water-percent",
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_CURRENT: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_AMPERE,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_CURRENT,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_VOLTAGE: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_VOLT,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_VOLTAGE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_LEVEL: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_PERCENT,
            accuracy_decimals=0,
            device_class=DEVICE_CLASS_BATTERY,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_REMAINING_TIME: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_HOUR,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_DURATION,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_TEMPERATURE: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        CONF_BATTERY_CAPACITY: _sensor_schema(
            BatteryOneHub,
            unit_of_measurement=UNIT_AMPERE_HOUR,
            accuracy_decimals=0,
            icon="mdi:battery-high",
        ),
    }
)


# The hub keeps one pointer per type, so a second entry would take over the first one's updates
FINAL_VALIDATE_SCHEMA = fv.unique_platform_key(
    domain="sensor",
    platform=DOMAIN,
    keys=(CONF_PARENT_ID, CONF_TYPE),
    message="Only one fendt_caravan sensor of type '{type}' is allowed per hub",
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PARENT_ID])
    setter = {
        CONF_TEMP_IN: hub.set_temp_in_sensor,
        CONF_TEMP_OUT: hub.set_temp_out_sensor,
        CONF_WATER_LEVEL: hub.set_water_level_sensor,
        CONF_BATTERY_CURRENT: hub.set_battery_current_sensor,
        CONF_BATTERY_VOLTAGE: hub.set_battery_voltage_sensor,
        CONF_BATTERY_LEVEL: hub.set_battery_level_sensor,
        CONF_BATTERY_REMAINING_TIME: hub.set_battery_remaining_time_sensor,
        CONF_BATTERY_TEMPERATURE: hub.set_battery_temperature_sensor,
        CONF_BATTERY_CAPACITY: hub.set_battery_capacity_sensor,
    }[config[CONF_TYPE]]
    var = await sensor.new_sensor(config)
    cg.add(setter(var))
