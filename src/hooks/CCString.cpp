#include <Geode/Geode.hpp>
#include <util.hpp>
#include <tracing.hpp>

// Credits to mat (https://github.com/matcool/geode-mods/blob/main/fast-format/main.cpp)
// Optimizes CCString::createWithFormat to not allocate 100kb on each call.
// On average was ~8% faster on my machine.

using namespace geode::prelude;

static bool initHook(CCString* self, const char* format, va_list args) {
    ZoneScoped;

    int size = std::vsnprintf(nullptr, 0, format, args);
    self->m_sString = gd::string(size + 1, '\0');
    std::vsnprintf(self->m_sString.data(), self->m_sString.size(), format, args);

    return true;
}

$execute {
    auto addr = GetProcAddress(GetModuleHandleW(L"libcocos2d.dll"), "?initWithFormatAndValist@CCString@cocos2d@@AEAA_NPEBDPEAD@Z");

    // auto start1 = benchTimer();
    // for (size_t i = 0; i < 65536; i++) {
    //     CCString::createWithFormat("%s-%d", "hello", 43);
    // }
    // auto took1 = benchTimer() - start1;

    auto hook = Mod::get()->hook(
        reinterpret_cast<void*>(addr),
        &initHook,
        "CCString::initWithFormatAndValist",
        tulip::hook::TulipConvention::Default
    ).unwrap();
    hook->setPriority(99999999);

    // auto start2 = benchTimer();
    // for (size_t i = 0; i < 65536; i++) {
    //     CCString::createWithFormat("%s-%d", "hello", 43);
    // }
    // auto took2 = benchTimer() - start2;

    // log::debug("Before hook: {}, after hook: {}", formatDuration(took1), formatDuration(took2));
}
