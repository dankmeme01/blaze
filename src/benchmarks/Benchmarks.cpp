#include "Benchmarks.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

#ifdef BLAZE_DEBUG

using namespace geode::prelude;

static void runBench() {
    blaze::bench::fpff();
}

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto menu = this->getChildByID("bottom-menu");
        if (!menu) return true;

        auto btn = Button::createWithSprite("fire.png"_spr, [](auto btn) {
            runBench();
        });
        menu->addChild(btn);
        menu->updateLayout();

        return true;
    }
};

#endif
