#include <Geode/Geode.hpp>
#include <Geode/modify/CCBMFontConfiguration.hpp>
#include <AsyncLoad/Fonts.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

// ios has init inlined :D
#ifdef __APPLE__
# define HOOK_CREATE
#endif

namespace blaze {

struct HookedCCBMFontConfig : Modify<HookedCCBMFontConfig, CCBMFontConfiguration> {
    static void onModify(auto& self) {
        auto fname =
#ifdef HOOK_CREATE
            "cocos2d::CCBMFontConfiguration::create";
#else
            "cocos2d::CCBMFontConfiguration::initWithFNTfile";
#endif

        LoadModule::get().addHooks(self, fname);
        (void) self.setHookPriority(fname, Priority::Replace);
    }

#ifdef HOOK_CREATE
    static CCBMFontConfiguration* create(const char* file) {
        auto config = AsyncLoad::loadFont(file, true).take();
        if (!config) {
            log::warn("loadFont(\"{}\") failed!", file);
            return nullptr;
        }

        config->autorelease();
        return config;
    }
#else
    $override
    bool initWithFNTfile(const char* file) {
        // since this is always called on main thread (by us too), use cache
        auto config = AsyncLoad::loadFont(file, true);
        if (!config) return false;

        this->moveFieldsFrom(config);

        return true;
    }
#endif

    void moveFieldsFrom(CCBMFontConfiguration* other) {
        m_pFontDefDictionary = std::exchange(other->m_pFontDefDictionary, nullptr);
        m_nCommonHeight = other->m_nCommonHeight;
        m_tPadding = other->m_tPadding;
        m_sAtlasName = std::move(other->m_sAtlasName);
        m_pKerningDictionary = std::exchange(other->m_pKerningDictionary, nullptr);
        m_pCharacterSet = std::exchange(other->m_pCharacterSet, nullptr);
    }
};

}
