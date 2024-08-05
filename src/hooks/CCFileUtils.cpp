// Hook to make CCFileUtils::getFileData thread-safe on Android

#include <Geode/Geode.hpp>

#ifdef GEODE_IS_ANDROID

#include <mutex>

using namespace geode::prelude;

static std::mutex mutex;

unsigned char* getFileDataHook(CCFileUtilsAndroid* fileUtils, const char* pszFileName, const char* pszMode, unsigned long* pSize) {
    std::lock_guard lock(mutex);
    return fileUtils->getFileData(pszFileName, pszMode, pSize);
}

$execute {
    void* handle = dlopen("libcocos2dcpp.so", RTLD_LAZY | RTLD_NOLOAD);
    void* addr = dlsym(handle, "_ZN7cocos2d18CCFileUtilsAndroid11getFileDataEPKcS2_Pm");

    if (!addr) {
        log::error("Failed to hook CCFileUtilsAndroid::getFileData, address is nullptr");
        return;
    }

    auto hook = Mod::get()->hook(
        addr,
        &getFileDataHook,
        "cocos2d::CCFileUtilsAndroid::getFileData",
        tulip::hook::TulipConvention::Default
    ).unwrap();
    hook->setPriority(-199999999);
}

#endif