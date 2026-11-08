import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_TYPE
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, AldeDeviceHub, FridgeDeviceHub

CONF_ALDE_HEATER_ELECTRICITY = "alde_heater_electricity"
CONF_FRIDGE_MODE = "fridge_mode"
CONF_FRIDGE_TEMPERATURE = "fridge_temperature"

# The C++ side maps the index of each option to the panel's value, so the options are fixed
SELECT_OPTIONS: dict[str, list[str]] = {
    CONF_ALDE_HEATER_ELECTRICITY: ["Off", "1 kW", "2 kW", "3 kW"],
    CONF_FRIDGE_MODE: ["Performance", "Quiet", "Boost"],
    CONF_FRIDGE_TEMPERATURE: ["1", "2", "3", "4", "5"],
}


def _select_schema(hub: MockObjClass) -> cv.Schema:
    return select.select_schema(select.ApplySelect).extend(
        {
            cv.Required(CONF_PARENT_ID): cv.use_id(hub),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        CONF_ALDE_HEATER_ELECTRICITY: _select_schema(AldeDeviceHub),
        CONF_FRIDGE_MODE: _select_schema(FridgeDeviceHub),
        CONF_FRIDGE_TEMPERATURE: _select_schema(FridgeDeviceHub),
    }
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PARENT_ID])
    selects = select.sub_selects(config, parent=hub)
    await selects.create(
        config,
        setter=getattr(hub, f"set_{config[CONF_TYPE]}_select"),
        options=SELECT_OPTIONS[config[CONF_TYPE]],
        apply="select_control(entity, index)",
    )
