#include "util.hpp"

#include <cstdlib>

#ifdef GEODE_IS_WINDOWS
# include <psapi.h>
# include <Windows.h>
#endif

namespace blaze {

void unreachable() {
#ifdef __clang__
    __builtin_unreachable();
#else
    __assume(false);
#endif
}

// Thanks prevter!
// https://github.com/Prevter/vapor/blob/main/src/main.cpp
size_t imageSize() {
    static size_t size = []() -> size_t {
#ifdef GEODE_IS_WINDOWS
        auto handle = GetModuleHandle(nullptr);
        if (!handle) return 0;

        MODULEINFO info;
        if (!GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info)))
            return 0;

        return info.SizeOfImage;
#elif defined(GEODE_IS_INTEL_MAC)
        return 0x980000;
#elif defined(GEODE_IS_ARM_MAC)
        return 0x8B0000;
#else
        return 0;
#endif
    }();

    return size;
}

size_t cocosImageSize() {
    static size_t size = []() -> size_t {
#ifdef GEODE_IS_WINDOWS
        auto handle = GetModuleHandle("libcocos2d.dll");
        if (!handle) return 0;

        MODULEINFO info;
        if (!GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info)))
            return 0;

        return info.SizeOfImage;
#elif defined(GEODE_IS_INTEL_MAC)
        return 0x980000;
#elif defined(GEODE_IS_ARM_MAC)
        return 0x8B0000;
#else
        return imageSize();
#endif
    }();

    return size;

}

}