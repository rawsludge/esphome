import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_PLUG
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, AldeDeviceHub, FridgeDeviceHub, MainControlUnitHub

CONF_POWER_STATUS = "power_status"
CONF_ALDE_STATUS = "alde_status"
CONF_FRIDGE_STATUS = "fridge_status"

CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_POWER_STATUS: binary_sensor.binary_sensor_schema(
            icon="mdi:power-plug",
            device_class=DEVICE_CLASS_PLUG,
        ).extend(
            {
                cv.Required(CONF_PARENT_ID): cv.use_id(MainControlUnitHub),
            }
        ),
        CONF_ALDE_STATUS: binary_sensor.binary_sensor_schema(
            icon="mdi:power-plug",
            device_class=DEVICE_CLASS_CONNECTIVITY,
        ).extend(
            {
                cv.Required(CONF_PARENT_ID): cv.use_id(AldeDeviceHub),
            }
        ),
        CONF_FRIDGE_STATUS: binary_sensor.binary_sensor_schema(
            icon="mdi:fridge-top",
            device_class=DEVICE_CLASS_CONNECTIVITY,
        ).extend(
            {
                cv.Required(CONF_PARENT_ID): cv.use_id(FridgeDeviceHub),
            }
        ),
    }
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(getattr(parent, f"set_{config[CONF_TYPE]}_binary_sensor")(var))
