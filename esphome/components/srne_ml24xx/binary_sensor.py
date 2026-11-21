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
    REG_FAULTS_LOW,
    REG_LOAD_AND_CHARGING_STATE,
    SRNE_ML24XX_CHILD_SCHEMA,
    register_binary_sensor,
    srne_ml24xx_ns,
)

DEPENDENCIES = ["srne_ml24xx"]

SrneMl24xxBinarySensor = srne_ml24xx_ns.class_(
    "SrneMl24xxBinarySensor", binary_sensor.BinarySensor
)

# Fault and warning bits B16 to B31 are bits 0 to 15 of 0x0121, B0 to B15 bits 0 to 15 of 0x0122
_FAULTS = (
    ("load_open_circuit", REG_FAULTS_HIGH, 15),
    ("induction_probe_damaged", REG_FAULTS_HIGH, 14),
    ("capacitor_over_voltage", REG_FAULTS_HIGH, 13),
    ("battery_reversely_connected", REG_FAULTS_HIGH, 12),
    ("battery_low_temperature_charging_stopped", REG_FAULTS_HIGH, 11),
    ("overcharge_protection", REG_FAULTS_HIGH, 10),
    ("battery_low_temperature_discharging_prohibited", REG_FAULTS_HIGH, 9),
    ("battery_high_temperature_discharging_prohibited", REG_FAULTS_HIGH, 8),
    ("no_battery_detected", REG_FAULTS_HIGH, 7),
    ("solar_panel_reversely_connected", REG_FAULTS_LOW, 12),
    ("solar_panel_working_point_over_voltage", REG_FAULTS_LOW, 11),
    ("photovoltaic_input_over_voltage", REG_FAULTS_LOW, 9),
    ("photovoltaic_input_over_power", REG_FAULTS_LOW, 7),
    ("battery_high_temperature_charging_prohibited", REG_FAULTS_LOW, 6),
    ("controller_temperature_too_high", REG_FAULTS_LOW, 5),
    ("load_over_power", REG_FAULTS_LOW, 4),
    ("load_short_circuit", REG_FAULTS_LOW, 3),
    ("battery_under_voltage", REG_FAULTS_LOW, 2),
    ("battery_over_voltage", REG_FAULTS_LOW, 1),
    ("battery_over_discharge", REG_FAULTS_LOW, 0),
)

# (key, register offset, bit, device class, entity category)
_FIELDS = (
    ("load_status", REG_LOAD_AND_CHARGING_STATE, 15, DEVICE_CLASS_POWER, cv.UNDEFINED),
    # B22: 0 battery powered, 1 mains powered
    ("mains_power_supply", REG_FAULTS_HIGH, 6, DEVICE_CLASS_POWER, cv.UNDEFINED),
    *(
        (key, reg, bit, DEVICE_CLASS_PROBLEM, ENTITY_CATEGORY_DIAGNOSTIC)
        for key, reg, bit in _FAULTS
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
