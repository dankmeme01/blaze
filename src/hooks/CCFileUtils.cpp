// Hook to make CCFileUtils::getFileData thread-safe on Android

#include <Geode/Geode.hpp>

#ifdef GEODE_IS_ANDROID

#include <mutex>

using namespace geode::prelude;

static std::mutex mutex;

using Func_t = unsigned char* (*)(CCFileUtilsAndroid*, const char*, const char*, unsigned long*, bool);
static Func_t g_funcAddr;

unsigned char* getFileDataHook(CCFileUtilsAndroid* fileUtils, const char* pszFileName, const char* pszMode, unsigned long* pSize, bool async) {
    std::lock_guard lock(mutex);
    return g_funcAddr(fileUtils, pszFileName, pszMode, pSize, false);
}

$execute {
    void* handle = dlopen("libcocos2dcpp.so", RTLD_LAZY | RTLD_NOLOAD);
    g_funcAddr = (Func_t) dlsym(handle, "_ZN7cocos2d18CCFileUtilsAndroid13doGetFileDataEPKcS2_Pmb");

    if (!g_funcAddr) {
        log::error("Failed to hook CCFileUtilsAndroid::doGetFileData, address is nullptr");
        return;
    }

    auto hook = Mod::get()->hook(
        (void*)g_funcAddr,
        &getFileDataHook,
        "cocos2d::CCFileUtilsAndroid::doGetFileData",
        tulip::hook::TulipConvention::Default
    ).unwrap();
    hook->setPriority(-199999999);
}

#endif