#include <Geode/Geode.hpp>
#include <util.hpp>
#include <tracing.hpp>

// Credits to mat (https://github.com/matcool/geode-mods/blob/main/fast-format/main.cpp)
// Optimizes CCString::createWithFormat to not allocate 100kb on each call.
// On average was ~8% faster on my machine.

using namespace geode::prelude;

static bool CCString_initHook(CCString* self, const char* format, va_list args) {
    ZoneScopedN("CCString::initWithFormatAndValist");

    // Note: this size excludes the null terminator
    int size = std::vsnprintf(nullptr, 0, format, args);

#ifndef GEODE_IS_ANDROID
    self->m_sString = gd::string(size, '\0');
#else
    self->m_sString = gd::string(std::string(size, '\0'));
#endif

    std::vsnprintf(self->m_sString.data(), self->m_sString.size() + 1, format, args);

    return true;
}

$execute {
#ifdef GEODE_IS_WINDOWS
    void* addr = reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"libcocos2d.dll"), "?initWithFormatAndValist@CCString@cocos2d@@AEAA_NPEBDPEAD@Z"));
#elif defined(GEODE_IS_ANDROID)
    void* handle = dlopen("libcocos2dcpp.so", RTLD_LAZY | RTLD_NOLOAD);
    void* addr = dlsym(handle, "_ZN7cocos2d8CCString23initWithFormatAndValistEPKcSt9__va_list");
#else
    void* addr = nullptr;
#endif
    if (!addr) {
        return;
    }

    // auto start1 = benchTimer();
    // for (size_t i = 0; i < 2000; i++) {
    //     CCString::createWithFormat("%d", i);
    // }
    // auto took1 = benchTimer() - start1;

    auto hook = Mod::get()->hook(
        addr,
        &CCString_initHook,
        "CCString::initWithFormatAndValist",
        tulip::hook::TulipConvention::Default
    ).unwrap();
    hook->setPriority(99999999);

    // auto start2 = benchTimer();
    // for (size_t i = 0; i < 2000; i++) {
    //     CCString::createWithFormat("%d", i);
    // }
    // auto took2 = benchTimer() - start2;

    // log::debug("Before hook: {}, after hook: {}", formatDuration(took1), formatDuration(took2));
}
