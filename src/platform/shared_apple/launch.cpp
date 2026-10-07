#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <modules/load/LoadModule.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(CCApplication) {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(self);

        (void) self.setHookPriority("cocos2d::CCApplication::run", Priority::First);
    }

    int run() {
        LoadModule::get().onEntry();
        return CCApplication::run();
    }
};

}

namespace blaze::platform {

// Same reasoning here as android
asp::Instant getProcessStartTime() {
    return asp::Instant::now();
}

}
