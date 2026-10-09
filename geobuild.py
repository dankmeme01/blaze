from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from .build.geobuild.prelude import *

def main(build: Build):
    config = build.config
    debug = build.add_option("BLAZE_DEBUG", False, "Enable debug mode for blaze")
    harden_alloc = build.add_option("BLAZE_HARDEN_ALLOC", False, "Enable harden mode for mimalloc, reducing performance by a few % to detect memory errors")
    debug_alloc = build.add_option("BLAZE_DEBUG_ALLOC", False, "Enable debug mode for mimalloc, makes the allocator very slow")
    guard_alloc = build.add_option("BLAZE_GUARD_ALLOC", False, "Enable guarded allocations in mimalloc, makes the allocator abysmally slow but best for catching memory errors")

    mimalloc_harden_level = 0
    if guard_alloc:
        mimalloc_harden_level = 3
    elif debug_alloc:
        mimalloc_harden_level = 2
    elif harden_alloc:
        mimalloc_harden_level = 1

    build.add_include_dir("src")
    # build.add_include_dir("include")
    build.add_source_dir("src/*.cpp", recursive=False)
    build.add_source_dir("src/modules/*.cpp", recursive=True)
    build.add_source_dir("src/benchmarks/*.cpp", recursive=True)
    build.add_source_dir("src/util/*.cpp", recursive=True)
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

    if config.platform.is_android():
        build.link_libraries("EGL")

    # epic deps
    build.add_cpm_dep("ebiggers/libdeflate", "v1.25", options={
        "LIBDEFLATE_BUILD_SHARED_LIB": "OFF",
        "LIBDEFLATE_BUILD_GZIP": "OFF",
    }, link_name="libdeflate_static")

    mi_opts = {
        "MI_OVERRIDE": "OFF",
        "MI_XMALLOC": "ON",
        "MI_SHOW_ERRORS": "OFF",
        "MI_FREE_IS_CHECKED": "OFF",
        "MI_DEBUG": "OFF",
        "MI_SECURE": "OFF",
        "MI_BUILD_SHARED": "OFF",
        "MI_BUILD_STATIC": "ON",
        "MI_BUILD_TESTS": "OFF",
        "MI_SKIP_COLLECT_ON_EXIT": "ON",
    }

    if mimalloc_harden_level >= 1:
        # some slight hardening
        mi_opts["MI_SECURE"] = "ON"

    if mimalloc_harden_level >= 2:
        # way more debugging, assertions, etc. fairly slow
        mi_opts["MI_DEBUG"] = "FULL"
        mi_opts["MI_SHOW_ERRORS"] = "ON"

    if mimalloc_harden_level >= 3:
        # highest and slowest level of security
        mi_opts["MI_SECURE"] = "FULL"
        mi_opts["MI_GUARDED"] = "ON"

    build.add_cpm_dep("microsoft/mimalloc", "v3.5.3", options=mi_opts, link_name="mimalloc-static")
    build.add_cpm_dep("greg7mdp/gtl", "v1.2.0", link_name="gtl", options={})

