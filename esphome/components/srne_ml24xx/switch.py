from esphome.automation import parent_ref
import esphome.codegen as cg
from esphome.components import switch
import esphome.config_validation as cv
from esphome.const import DEVICE_CLASS_SWITCH, ENTITY_CATEGORY_CONFIG
from esphome.types import ConfigType

from . import (
    BLOCK_SETTINGS,
    CONF_SRNE_ML24XX_ID,
    SETTING_SPECIAL_POWER_CONTROL,
    SETTINGS_PUBLISHER,
    SRNE_ML24XX_CHILD_SCHEMA,
    request_block,
    setting,
)

DEPENDENCIES = ["srne_ml24xx"]

CONF_LOAD = "load"

# Bits of E021 (special power control): key: bit
_SETTING_BITS = {
    # Clears the over-discharge mark each night, so the load can turn on at least once
    "each_night_on": 8,
    "special_power_control": 9,
    "no_charging_below_zero": 2,
}

_CONFIG_SWITCH_SCHEMA = switch.switch_schema(
    switch.ApplySwitch, entity_category=ENTITY_CATEGORY_CONFIG
)

CONFIG_SCHEMA = cv.All(
    SRNE_ML24XX_CHILD_SCHEMA.extend(
        {
            # Only acts when the load working mode is manual
            cv.Optional(CONF_LOAD): switch.switch_schema(
                switch.ApplySwitch, device_class=DEVICE_CLASS_SWITCH
            ),
            **{cv.Optional(key): _CONFIG_SWITCH_SCHEMA for key in _SETTING_BITS},
        }
    ),
    cv.has_at_least_one_key(CONF_LOAD, *_SETTING_BITS),
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_SRNE_ML24XX_ID])
    switches = switch.sub_switches(config, parent=hub)
    await switches(
        CONF_LOAD, hub.set_load_switch, apply="set_load(state)", publish="on_success"
    )
    for key, bit in _SETTING_BITS.items():
        mask = 1 << bit
        var = await switches(
            key,
            None,
            apply=(
                f"write_setting_bits({SETTING_SPECIAL_POWER_CONTROL:#06x}, "
                f"{mask:#06x}, state ? {mask:#06x} : 0)"
            ),
            publish="on_success",
        )
        if var is not None:
            request_block(hub, BLOCK_SETTINGS)
            state = (setting(SETTING_SPECIAL_POWER_CONTROL) >> bit) & 1
            SETTINGS_PUBLISHER.add(hub, parent_ref(var).publish_state(state != 0))
