#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <modules/load/LoadModule.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(CCApplication) {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "cocos2d::CCApplication::run"
        );

        (void) self.setHookPriority("cocos2d::CCApplication::run", Priority::First);
    }

    int run() {
        LoadModule::get().onEntry();
        return CCApplication::run();
    }
};

}
