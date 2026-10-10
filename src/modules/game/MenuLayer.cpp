#include "GameModule.hpp"
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(MenuLayer) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);
    }

    bool init() {
        arc::spawnBlocking<void>([] {
            GameModule::get().populdateSpriteFrames();
        });

        return MenuLayer::init();
    }
};

}
