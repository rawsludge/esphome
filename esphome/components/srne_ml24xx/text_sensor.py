import esphome.codegen as cg
from esphome.components import text_sensor
from esphome.components.const import ICON_SOLAR_POWER
import esphome.config_validation as cv
from esphome.types import ConfigType

from . import CONF_SRNE_ML24XX_ID, SRNE_ML24XX_CHILD_SCHEMA

DEPENDENCIES = ["srne_ml24xx"]

CONF_CHARGING_STATE = "charging_state"

CONFIG_SCHEMA = SRNE_ML24XX_CHILD_SCHEMA.extend(
    {
        cv.Required(CONF_CHARGING_STATE): text_sensor.text_sensor_schema(
            icon=ICON_SOLAR_POWER
        ),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    sensors = text_sensor.sub_text_sensors(config)
    await sensors(CONF_CHARGING_STATE, hub.set_charging_state_text_sensor)
