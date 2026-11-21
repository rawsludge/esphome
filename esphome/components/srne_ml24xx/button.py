from esphome.components import button
import esphome.config_validation as cv
from esphome.const import CONF_FACTORY_RESET, ENTITY_CATEGORY_CONFIG
from esphome.types import ConfigType

from . import CONF_SRNE_ML24XX_ID, SRNE_ML24XX_CHILD_SCHEMA

DEPENDENCIES = ["srne_ml24xx"]

CONF_CLEAR_HISTORY = "clear_history"

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            cv.Optional(CONF_FACTORY_RESET): button.factory_reset_button_schema(
                button.ApplyButton
            ),
            cv.Optional(CONF_CLEAR_HISTORY): button.button_schema(
                button.ApplyButton,
                entity_category=ENTITY_CATEGORY_CONFIG,
                icon="mdi:delete-clock",
            ),
        }
    ),
    cv.has_at_least_one_key(CONF_FACTORY_RESET, CONF_CLEAR_HISTORY),
)


async def to_code(config: ConfigType) -> None:
    buttons = button.sub_buttons(config, parent=config[CONF_SRNE_ML24XX_ID])
    await buttons(CONF_FACTORY_RESET, None, apply="factory_reset()")
    await buttons(CONF_CLEAR_HISTORY, None, apply="clear_history()")
