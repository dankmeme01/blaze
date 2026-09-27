#include <Geode/Geode.hpp>
#include <Geode/modify/CCBMFontConfiguration.hpp>
#include <AsyncLoad/Fonts.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

namespace blaze {

struct HookedCCBMFontConfig : Modify<HookedCCBMFontConfig, CCBMFontConfiguration> {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "cocos2d::CCBMFontConfiguration::initWithFNTfile"
        );

        (void) self.setHookPriority("cocos2d::CCBMFontConfiguration::initWithFNTfile", Priority::Replace);
    }

    $override
    bool initWithFNTfile(const char* file) {
        // since this is always called on main thread (by us too), use cache
        auto config = AsyncLoad::loadFont(file, true);
        if (!config) return false;

        this->moveFieldsFrom(config);

        return true;
    }

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
