import esphome.codegen as cg
from esphome.components import select
import esphome.config_validation as cv
from esphome.const import CONF_OPTIONS, CONF_TYPE

from .. import CONF_PARENT_ID, FendtCaravanHubBase, fendt_caravan_ns

FendtSelect = fendt_caravan_ns.class_(
    "FendtSelect",
    select.Select,
    cg.Parented.template(FendtCaravanHubBase),
)


def _select_schema(icon: str = cv.UNDEFINED) -> cv.Schema:
    return select.select_schema(FendtSelect).extend(
        {
            cv.Required(CONF_PARENT_ID): cv.use_id(FendtCaravanHubBase),
        }
    )


CONFIG_SCHEMA = cv.typed_schema(
    {
        "alde_heater_electricity": _select_schema().extend(
            {
                cv.Optional(
                    CONF_OPTIONS, default=["Off", "1 kW", "2 kW", "3 kW"]
                ): cv.ensure_list(cv.string_strict)
            }
        )
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_PARENT_ID])
    var = await select.new_select(config, options=config[CONF_OPTIONS])
    await cg.register_parented(var, parent)
    cg.add(getattr(parent, f"set_{config[CONF_TYPE]}_select")(var))
