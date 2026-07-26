from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from .build.geobuild.prelude import *

def main(build: Build):
    config = build.config
    debug = build.add_option("BLAZE_DEBUG", False, "Enable debug mode for blaze")

    build.add_include_dir("src")
    # build.add_include_dir("include")
    build.add_source_dir("src/*.cpp", recursive=False)
    build.add_source_dir("src/modules/*.cpp", recursive=True)
    build.add_source_dir(f"src/platform/{config.platform.platform_str(False)}/")

    build.enable_mod_json_generation("mod.template.json")
    build.add_geode_dep("dankmeme.async-load-api", {
        "version": ">=v1.0.0",
        "required": True,
    })
    build.add_include_dir(build.config.build_dir / "geode-deps" / "dankmeme.async-load-api" / "include")
    build.relax_geode_requirement()

    if config.platform.is_apple():
        build.add_source_dir("src/platform/shared_apple")

    if debug:
        build.add_definition("BLAZE_DEBUG")

