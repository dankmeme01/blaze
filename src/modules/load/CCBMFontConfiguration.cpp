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
        auto config = AsyncLoad::loadFont(file);
        if (!config) return false;

        this->copyFrom(config);

        return true;
    }

    void copyFrom(CCBMFontConfiguration* other) {
        m_pFontDefDictionary = other->m_pFontDefDictionary;
        m_nCommonHeight = other->m_nCommonHeight;
        m_tPadding = other->m_tPadding;
        m_sAtlasName = other->m_sAtlasName;
        m_pKerningDictionary = other->m_pKerningDictionary;
        m_pCharacterSet = other->m_pCharacterSet;
    }
};

}
