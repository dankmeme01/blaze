#include "GameModule.hpp"
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(GameManager) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("GameManager::setIntGameVariable", Priority::Replace * 100);
    }

    // rewrite to not use ccstring::create (thread-safe if no one else accesses the gmanager)
    $override
    void setIntGameVariable(char const* keyR, int valueR) {
        auto key = fmt::format("gv_{}", keyR);
        auto value = fmt::to_string(valueR);

        auto valstr = Ref<CCString>::adopt(new CCString());
        valstr->m_sString = value;

        m_valueKeeper->setObject(valstr, key);
    }
};

}
