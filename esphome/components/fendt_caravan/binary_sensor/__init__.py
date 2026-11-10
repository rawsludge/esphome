import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_PLUG
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, AldeDeviceHub, FridgeDeviceHub, MainControlUnitHub

CONF_POWER_STATUS = "power_status"
CONF_ALDE_STATUS = "alde_status"
CONF_FRIDGE_STATUS = "fridge_status"


def _binary_sensor_schema(
    hub: MockObjClass,
    device_class: str = cv.UNDEFINED,
    icon: str = cv.UNDEFINED,
) -> cv.Schema:
    return binary_sensor.binary_sensor_schema(
        device_class=device_class,
        icon=icon,
    ).extend(
        {
            cv.Required(CONF_PARENT_ID): cv.use_id(hub),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_POWER_STATUS: _binary_sensor_schema(
            MainControlUnitHub, device_class=DEVICE_CLASS_PLUG, icon="mdi:power-plug"
        ),
        CONF_ALDE_STATUS: _binary_sensor_schema(
            AldeDeviceHub,
            device_class=DEVICE_CLASS_CONNECTIVITY,
            icon="mdi:power-plug",
        ),
        CONF_FRIDGE_STATUS: _binary_sensor_schema(
            FridgeDeviceHub,
            device_class=DEVICE_CLASS_CONNECTIVITY,
            icon="mdi:fridge-top",
        ),
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(getattr(parent, f"set_{config[CONF_TYPE]}_binary_sensor")(var))
