import esphome.codegen as cg
from esphome.components import ble_client, esp32_ble_tracker
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.types import ConfigType

AUTO_LOAD = ["binary_sensor", "sensor", "switch", "text_sensor"]
CODEOWNERS = ["@rawsludge"]
DEPENDENCIES = ["ble_client", "esp32_ble_tracker"]
DOMAIN = "fendt_caravan"

MULTI_CONF = True

fendt_caravan_ns = cg.esphome_ns.namespace("fendt_caravan")
FendtCaravan = fendt_caravan_ns.class_(
    "FendtCaravan", ble_client.BLEClientNode, cg.Component
)
FendtCaravanHubBase = fendt_caravan_ns.class_(
    "FendtCaravanHubBase", cg.PollingComponent
)
MainControlUnitHub = fendt_caravan_ns.class_("MainControlUnitHub", FendtCaravanHubBase)
AldeDeviceHub = fendt_caravan_ns.class_("AldeDeviceHub", FendtCaravanHubBase)
FridgeDeviceHub = fendt_caravan_ns.class_("FridgeDeviceHub", FendtCaravanHubBase)
LightingDeviceHub = fendt_caravan_ns.class_("LightingDeviceHub", FendtCaravanHubBase)

CONF_BT_ID = "bt_id"
CONF_PARENT_ID = "parent_id"
CONF_FRIDGE_STATUS = "fridge_status"
CONF_MAIN_CONTROL_UNIT = "main_control_unit"
CONF_ALDE_UNIT = "alde_unit"
CONF_FRIDGE_UNIT = "fridge_unit"
CONF_LIGHTING_UNIT = "lighting_unit"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(FendtCaravan),
            # The address the panel was paired with; it drops the link when given any other one
            cv.Required(CONF_BT_ID): cv.mac_address,
            cv.Required(ble_client.CONF_BLE_CLIENT_ID): cv.use_id(ble_client.BLEClient),
            cv.Required(CONF_MAIN_CONTROL_UNIT): cv.Schema(
                {
                    cv.GenerateID(): cv.declare_id(MainControlUnitHub),
                }
            ).extend(cv.polling_component_schema("60s")),
            cv.Optional(CONF_ALDE_UNIT): cv.Schema(
                {
                    cv.GenerateID(): cv.declare_id(AldeDeviceHub),
                }
            ).extend(cv.polling_component_schema("60s")),
            cv.Optional(CONF_FRIDGE_UNIT): cv.Schema(
                {
                    cv.GenerateID(): cv.declare_id(FridgeDeviceHub),
                }
            ).extend(cv.polling_component_schema("60s")),
            cv.Required(CONF_LIGHTING_UNIT): cv.Schema(
                {
                    cv.GenerateID(): cv.declare_id(LightingDeviceHub),
                }
            ).extend(cv.polling_component_schema("60s")),
        }
    ).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA)
)


async def to_code(config: ConfigType) -> None:
    # The handshake carries the paired address in lower case
    bt_id = ":".join(f"{part:02x}" for part in config[CONF_BT_ID].parts)
    var = await ble_client.new_ble_node_component(config, f"net-BT_ID-{bt_id}")
    hub = cg.new_Pvariable(config[CONF_MAIN_CONTROL_UNIT][CONF_ID])
    await cg.register_parented(hub, var)
    await cg.register_component(hub, config[CONF_MAIN_CONTROL_UNIT])
    cg.add(var.set_mcu_hub(hub))
    if CONF_ALDE_UNIT in config:
        hub = cg.new_Pvariable(config[CONF_ALDE_UNIT][CONF_ID])
        await cg.register_parented(hub, var)
        await cg.register_component(hub, config[CONF_ALDE_UNIT])
        cg.add(var.set_alde_unit_hub(hub))
    if CONF_FRIDGE_UNIT in config:
        hub = cg.new_Pvariable(config[CONF_FRIDGE_UNIT][CONF_ID])
        await cg.register_parented(hub, var)
        await cg.register_component(hub, config[CONF_FRIDGE_UNIT])
        cg.add(var.set_fridge_unit_hub(hub))
    if CONF_LIGHTING_UNIT in config:
        hub = cg.new_Pvariable(config[CONF_LIGHTING_UNIT][CONF_ID])
        await cg.register_parented(hub, var)
        await cg.register_component(hub, config[CONF_LIGHTING_UNIT])
        cg.add(var.set_lighting_unit_hub(hub))
