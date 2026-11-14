from esphome import final_validate as fv
import esphome.codegen as cg
from esphome.components import binary_sensor
import esphome.config_validation as cv
from esphome.const import CONF_TYPE, DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_PLUG
from esphome.cpp_generator import MockObjClass
from esphome.types import ConfigType

from .. import (
    CONF_FRIDGE_STATUS,
    CONF_PARENT_ID,
    DOMAIN,
    AldeDeviceHub,
    BatteryOneHub,
    FridgeDeviceHub,
    MainControlUnitHub,
)

CONF_POWER_STATUS = "power_status"
CONF_ALDE_STATUS = "alde_status"
CONF_BATTERY_AVAILABLE = "battery_available"
CONF_BATTERY_RECALIBRATED = "battery_recalibrated"


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
        CONF_BATTERY_AVAILABLE: _binary_sensor_schema(
            BatteryOneHub,
            device_class=DEVICE_CLASS_CONNECTIVITY,
            icon="mdi:battery",
        ),
        CONF_BATTERY_RECALIBRATED: _binary_sensor_schema(
            BatteryOneHub, icon="mdi:battery-sync"
        ),
    }
)


# The hub keeps one pointer per type, so a second entry would take over the first one's updates
FINAL_VALIDATE_SCHEMA = fv.unique_platform_key(
    domain="binary_sensor",
    platform=DOMAIN,
    keys=(CONF_PARENT_ID, CONF_TYPE),
    message="Only one fendt_caravan binary sensor of type '{type}' is allowed per hub",
)


async def to_code(config: ConfigType) -> None:
    hub = await cg.get_variable(config[CONF_PARENT_ID])
    setter = {
        CONF_POWER_STATUS: hub.set_power_status_binary_sensor,
        CONF_ALDE_STATUS: hub.set_alde_status_binary_sensor,
        CONF_FRIDGE_STATUS: hub.set_fridge_status_binary_sensor,
        CONF_BATTERY_AVAILABLE: hub.set_battery_available_binary_sensor,
        CONF_BATTERY_RECALIBRATED: hub.set_battery_recalibrated_binary_sensor,
    }[config[CONF_TYPE]]
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(setter(var))
