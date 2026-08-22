from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from .build.geobuild.prelude import *

def main(build: Build):
    config = build.config
    debug = build.add_option("BLAZE_DEBUG", False, "Enable debug mode for blaze")
    harden_alloc = build.add_option("BLAZE_HARDEN_ALLOC", False, "Enable harden mode for mimalloc, reducing performance by a few % to detect memory errors")
    debug_alloc = build.add_option("BLAZE_DEBUG_ALLOC", False, "Enable debug mode for mimalloc, makes the allocator very slow!!!")

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

    # epic deps
    build.add_cpm_dep("ebiggers/libdeflate", "v1.25", options={
        "LIBDEFLATE_BUILD_SHARED_LIB": "OFF",
        "LIBDEFLATE_BUILD_GZIP": "OFF",
    }, link_name="libdeflate_static")

    # use tag till v3.5.1 or such is released: https://github.com/microsoft/mimalloc/issues/1370
    build.add_cpm_dep("microsoft/mimalloc", "622d421", options={
        "MI_OVERRIDE": "OFF",
        "MI_XMALLOC": "ON",
        "MI_SHOW_ERRORS": "ON" if debug else "OFF",
        "MI_FREE_IS_CHECKED": "OFF",
        "MI_DEBUG": "FULL" if debug_alloc else "OFF",
        "MI_SECURE": "ON" if harden_alloc else "OFF",
        "MI_BUILD_SHARED": "OFF",
        "MI_BUILD_STATIC": "ON",
        "MI_BUILD_TESTS": "OFF",
        "MI_SKIP_COLLECT_ON_EXIT": "ON",
    }, link_name="mimalloc-static")

