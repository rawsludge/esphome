from functools import partial

import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import (
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_DIAGNOSTIC,
)
from esphome.types import ConfigType

from . import (
    CONF_SRNE_ML24XX_ID,
    REG_FAULTS_HIGH,
    REG_LOAD_AND_CHARGING_STATE,
    SRNE_ML24XX_CHILD_SCHEMA,
    register_binary_sensor,
    srne_ml24xx_ns,
)

DEPENDENCIES = ["srne_ml24xx"]

SrneMl24xxBinarySensor = srne_ml24xx_ns.class_(
    "SrneMl24xxBinarySensor", binary_sensor.BinarySensor
)

# Fault bits B16 to B30 of the 32-bit value in 0x0121/0x0122 are bits 0 to 14 of 0x0121
_FAULTS = (
    ("charge_mos_short_circuit", 14),
    ("anti_reverse_mos_short_circuit", 13),
    ("solar_panel_reversely_connected", 12),
    ("solar_panel_working_point_over_voltage", 11),
    ("solar_panel_counter_current", 10),
    ("photovoltaic_input_over_voltage", 9),
    ("photovoltaic_input_short_circuit", 8),
    ("photovoltaic_input_over_power", 7),
    ("ambient_temperature_too_high", 6),
    ("controller_temperature_too_high", 5),
    ("load_over_power", 4),
    ("load_short_circuit", 3),
    ("battery_under_voltage", 2),
    ("battery_over_voltage", 1),
    ("battery_over_discharge", 0),
)

# (key, register offset, bit, device class, entity category)
_FIELDS = (
    ("load_status", REG_LOAD_AND_CHARGING_STATE, 15, DEVICE_CLASS_POWER, cv.UNDEFINED),
    *(
        (key, REG_FAULTS_HIGH, bit, DEVICE_CLASS_PROBLEM, ENTITY_CATEGORY_DIAGNOSTIC)
        for key, bit in _FAULTS
    ),
)

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            cv.Optional(key): binary_sensor.binary_sensor_schema(
                SrneMl24xxBinarySensor,
                device_class=device_class,
                entity_category=entity_category,
            )
            for key, _, _, device_class, entity_category in _FIELDS
        }
    ),
    cv.has_at_least_one_key(*(key for key, *_ in _FIELDS)),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    sensors = binary_sensor.sub_binary_sensors(config)
    for key, reg, bit, *_ in _FIELDS:
        await sensors(key, partial(register_binary_sensor, hub), reg, bit)
