import esphome.codegen as cg
from esphome.core import CORE
from esphome.types import ConfigType
from tests.testing_helpers import ComponentManifestOverride


def override_manifest(manifest: ComponentManifestOverride) -> None:
    async def to_code_testing(config: ConfigType) -> None:
        for platform in ("sensor", "binary_sensor", "text_sensor"):
            CORE.testing_ensure_platform_registered(platform)
        cg.add_define("SRNE_ML24XX_SENSOR_COUNT", 8)
        cg.add_define("SRNE_ML24XX_BINARY_SENSOR_COUNT", 4)

    # A MULTI_CONF component gets no entry in the host build, so its to_code would never run.
    manifest.multi_conf = False
    manifest.to_code = to_code_testing
    manifest.dependencies = manifest.dependencies + [
        "sensor",
        "binary_sensor",
        "text_sensor",
    ]
