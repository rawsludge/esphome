import esphome.codegen as cg
from esphome.components import light
import esphome.config_validation as cv
from esphome.const import CONF_DEFAULT_TRANSITION_LENGTH, CONF_ID, CONF_OUTPUT_ID
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import CONF_PARENT_ID, LightingDeviceHub, fendt_caravan_ns

CONF_LIGHT_TYPE = "light_type"
CONF_VARIABLE_NAME = "variable_name"

LIGHT_TYPE_SWITCH = "switch"
LIGHT_TYPE_DIMMABLE = "dimmable"

FendtBinaryLightOutput = fendt_caravan_ns.class_(
    "FendtBinaryLightOutput",
    light.LightOutput,
    cg.Parented.template(LightingDeviceHub),
)

FendtFloatLightOutput = fendt_caravan_ns.class_(
    "FendtFloatLightOutput",
    light.LightOutput,
    cg.Parented.template(LightingDeviceHub),
)

_request_binary_light_slot = cg.slot_counter("LIGHTING_DEVICE_BINARY_LIGHT_COUNT")
_request_float_light_slot = cg.slot_counter("LIGHTING_DEVICE_FLOAT_LIGHT_COUNT")


def _light_schema(base: cv.Schema, output_class: MockObjClass) -> cv.Schema:
    return base.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(output_class),
            cv.Required(CONF_PARENT_ID): cv.use_id(LightingDeviceHub),
            cv.Required(CONF_VARIABLE_NAME): cv.string,
        }
    ).extend(cv.COMPONENT_SCHEMA)


CONFIG_SCHEMA = cv.typed_schema(
    {
        LIGHT_TYPE_SWITCH: _light_schema(
            light.BINARY_LIGHT_SCHEMA, FendtBinaryLightOutput
        ),
        LIGHT_TYPE_DIMMABLE: _light_schema(
            light.BRIGHTNESS_ONLY_LIGHT_SCHEMA, FendtFloatLightOutput
        ).extend(
            {
                cv.Optional(
                    CONF_DEFAULT_TRANSITION_LENGTH, default="0s"
                ): cv.positive_time_period_milliseconds,
            }
        ),
    },
    key=CONF_LIGHT_TYPE,
    lower=True,
)


async def to_code(config: ConfigType) -> None:
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await light.register_light(var, config)
    await cg.register_parented(var, parent)
    state = await cg.get_variable(config[CONF_ID])
    name = config[CONF_VARIABLE_NAME]
    cg.add(var.set_variable_name(name))
    if config[CONF_LIGHT_TYPE] == LIGHT_TYPE_SWITCH:
        _request_binary_light_slot()
        cg.add(parent.set_binary_light_state(name, state))
    else:
        _request_float_light_slot()
        cg.add(parent.set_float_light_state(name, state))
