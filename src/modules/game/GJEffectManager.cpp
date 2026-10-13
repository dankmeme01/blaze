#include <Geode/Geode.hpp>
#include <Geode/modify/GJEffectManager.hpp>
#include "GameModule.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(GJEffectManager) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("GJEffectManager::calculateBaseActiveColors", Priority::Replace * 100);
    }


    // replace slow allKeys()  with normal dict iteration
    void calculateBaseActiveColors() {
        for (auto [k, sprite] : CCDictionaryExt<int, ColorActionSprite>(m_unkDict150)) {
            auto action = sprite->m_colorAction;
            ccColor3B color = sprite->m_color;
            if (action && action->m_copyID > 0 && !action->m_copyColorLoop) {
                color = GameToolbox::transformColor(color, action->m_copyHSV);
            }
            sprite->m_copyColor = color;
        }
    }
};

}
