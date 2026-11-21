import esphome.codegen as cg
from esphome.components import text_sensor
from esphome.components.const import CONF_SERIAL_NUMBER, ICON_SOLAR_POWER
import esphome.config_validation as cv
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC, ICON_CHIP
from esphome.types import ConfigType

from . import (
    BLOCK_SYSTEM_INFO,
    CONF_SRNE_ML24XX_ID,
    SRNE_ML24XX_CHILD_SCHEMA,
    request_block,
)

DEPENDENCIES = ["srne_ml24xx"]

CONF_CHARGING_STATE = "charging_state"
CONF_HARDWARE_VERSION = "hardware_version"
CONF_PRODUCT_MODEL = "product_model"
CONF_PRODUCT_TYPE = "product_type"
CONF_SOFTWARE_VERSION = "software_version"

_SYSTEM_INFO_KEYS = (
    CONF_PRODUCT_MODEL,
    CONF_PRODUCT_TYPE,
    CONF_SOFTWARE_VERSION,
    CONF_HARDWARE_VERSION,
    CONF_SERIAL_NUMBER,
)

_DIAGNOSTIC_SCHEMA = text_sensor.text_sensor_schema(
    icon=ICON_CHIP, entity_category=ENTITY_CATEGORY_DIAGNOSTIC
)

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            cv.Optional(CONF_CHARGING_STATE): text_sensor.text_sensor_schema(
                icon=ICON_SOLAR_POWER
            ),
            **{cv.Optional(key): _DIAGNOSTIC_SCHEMA for key in _SYSTEM_INFO_KEYS},
        }
    ),
    cv.has_at_least_one_key(CONF_CHARGING_STATE, *_SYSTEM_INFO_KEYS),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    sensors = text_sensor.sub_text_sensors(config)
    await sensors(CONF_CHARGING_STATE, hub.set_charging_state_text_sensor)
    await sensors(CONF_PRODUCT_MODEL, hub.set_product_model_text_sensor)
    await sensors(CONF_PRODUCT_TYPE, hub.set_product_type_text_sensor)
    await sensors(CONF_SOFTWARE_VERSION, hub.set_software_version_text_sensor)
    await sensors(CONF_HARDWARE_VERSION, hub.set_hardware_version_text_sensor)
    await sensors(CONF_SERIAL_NUMBER, hub.set_serial_number_text_sensor)
    if any(key in config for key in _SYSTEM_INFO_KEYS):
        request_block(hub, BLOCK_SYSTEM_INFO)
