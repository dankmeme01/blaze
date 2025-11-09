from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .build.geobuild.prelude import *

def main(build: Build):
    config = build.config

    build.add_option("BLAZE_USE_TRACY", False, "Use tracy profiler")
    build.add_option("BLAZE_DEBUG", False, "Debug build")
    build.add_option("BLAZE_RELEASE", False, "Release build")

    build.add_source_dir("src/*.cpp")
    build.add_source_dir("external/")

    if build.platform.is_apple():
        build.add_source_file("src/hooks/load.mm")
        build.add_raw_statement("set_source_files_properties(${OBJC_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)")

    build.set_cache_variable("GEODE_DISABLE_PRECOMPILED_HEADERS", "ON", "BOOL", force=True)

    build.add_include_dir("external/")
    build.add_include_dir("src/")

    # make spng use miniz instead of zlib
    build.add_definition("SPNG_USE_MINIZ", "1")
    build.add_definition("SPNG_STATIC", "1")

    build.add_cpm_dep("dankmeme01/asp2", "d60ad30", link_name="asp")
    build.add_cpm_dep("simdutf/simdutf", "b74c852")
    build.add_cpm_dep("ebiggers/libdeflate", "v1.25", options={
        "LIBDEFLATE_BUILD_SHARED_LIB": "OFF",
        "LIBDEFLATE_BUILD_GZIP": "OFF",
    }, link_name="libdeflate_static")
    build.add_cpm_dep("Prevter/sinaps", "2541d6d")
    build.add_cpm_dep("zeux/pugixml", "v1.15", link_name="pugixml-static")
    build.add_cpm_dep("dankmeme01/fast_float", "b7b17e6")

    if config.bool_var("BLAZE_DEBUG"):
        build.add_definition("BLAZE_DEBUG", "1")
        build.add_definition("_HAS_ITERATOR_DEBUGGING", "0", target="asp")
        build.add_definition("_HAS_ITERATOR_DEBUGGING", "0", target="simdutf")

    if config.bool_var("BLAZE_RELEASE"):
        build.enable_lto()

    # silly thing
    if build.platform == Platform.Android64:
        build.add_compile_option("-march=armv8-a+crc")

    if config.bool_var("BLAZE_USE_TRACY"):
        # taken from https://github.com/cgytrus/AlgebraDash/blob/main/CMakeLists.txt
        build.set_cache_variable("TRACY_ENABLE", "ON", "BOOL")
        build.set_cache_variable("TRACY_ON_DEMAND", "ON", "BOOL")
        build.set_cache_variable("TRACY_ONLY_LOCALHOST", "OFF", "BOOL")
        build.set_cache_variable("TRACY_NO_CALLSTACK", "OFF", "BOOL")
        build.set_cache_variable("TRACY_CALLSTACK", "OFF", "BOOL")

        build.add_cpm_dep("dankmeme01/tracy", "95a3b94", link_name="Tracy::TracyClient")
        build.add_include_dir("${Tracy_SOURCE_DIR}/public/tracy")
        build.add_definition("BLAZE_TRACY", "1")
        build.add_compile_option("-Wno-incompatible-pointer-types")
