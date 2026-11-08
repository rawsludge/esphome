import esphome.codegen as cg
from esphome.components import text_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, FridgeDeviceHub, MainControlUnitHub

CONF_SOFTWARE_VERSION = "software_version"
CONF_FRIDGE_POWER_SOURCE = "fridge_power_source"
CONF_FRIDGE_TYPE = "fridge_type"
CONF_PANEL_VERSION = "panel_version"


def _text_schema(
    hub: MockObjClass,
    device_class: str = cv.UNDEFINED,
    entity_category: str = cv.UNDEFINED,
    icon: str = cv.UNDEFINED,
) -> cv.Schema:
    return text_sensor.text_sensor_schema(
        device_class=device_class,
        entity_category=entity_category,
        icon=icon,
    ).extend(
        {
            cv.Required(CONF_PARENT_ID): cv.use_id(hub),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_SOFTWARE_VERSION: _text_schema(
            MainControlUnitHub, icon="mdi:application-braces-outline"
        ),
        CONF_FRIDGE_POWER_SOURCE: _text_schema(
            FridgeDeviceHub, icon="mdi:power-plug-battery"
        ),
        CONF_FRIDGE_TYPE: _text_schema(FridgeDeviceHub, icon="mdi:fridge-top"),
        CONF_PANEL_VERSION: _text_schema(MainControlUnitHub, icon="mdi:monitor"),
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = await text_sensor.new_text_sensor(config)
    cg.add(getattr(parent, f"set_{config[CONF_TYPE]}_text_sensor")(var))
