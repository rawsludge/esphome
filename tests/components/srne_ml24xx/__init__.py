import esphome.codegen as cg
from esphome.core import CORE
from esphome.types import ConfigType
from tests.testing_helpers import ComponentManifestOverride

_PLATFORMS = ("sensor", "binary_sensor", "text_sensor", "switch")


def override_manifest(manifest: ComponentManifestOverride) -> None:
    async def to_code_testing(config: ConfigType) -> None:
        for platform in _PLATFORMS:
            CORE.testing_ensure_platform_registered(platform)
        cg.add_define("SRNE_ML24XX_SENSOR_COUNT", 8)
        cg.add_define("SRNE_ML24XX_BINARY_SENSOR_COUNT", 4)
        cg.add_define("SRNE_ML24XX_HISTORY_DAY_COUNT", 2)
        cg.add_define("USE_SRNE_ML24XX_SETTINGS_PUBLISHER")

    # A MULTI_CONF component gets no entry in the host build, so its to_code would never run.
    manifest.multi_conf = False
    manifest.to_code = to_code_testing
    manifest.dependencies = manifest.dependencies + list(_PLATFORMS)
