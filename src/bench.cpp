#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <TaskTimer.hpp>

#include <hooks/load/spriteframes.hpp>
#include <fpff.hpp>

using namespace geode::prelude;

#ifdef BLAZE_DEBUG

static void benchSpriteFrames() {
    auto texture = CCTextureCache::get()->textureForKey("PixelSheet_01.png");
    if (!texture) {
        log::error("Error: failed to load texture");
        return;
    }

    // Test our functions

    CCSpriteFrameCache::purgeSharedSpriteFrameCache();
    auto cache = CCSpriteFrameCache::get();

    BLAZE_TIMER_START("Parse sprite frames (custom)");

    auto fp = blaze::fullPathForFilename("PixelSheet_01.plist", false);
    unsigned long size;
    auto data = CCFileUtils::get()->getFileData(fp.c_str(), "rt", &size);

    if (!size || !data) {
        log::error("Error: failed to load sprite frames");
        return;
    }

    auto spf = blaze::parseSpriteFrames(data, size);
    if (!spf) {
        log::error("Error: failed to parse sprite frames: {}", spf.unwrapErr());
        return;
    }

    BLAZE_TIMER_STEP("Add sprite frames (custom)");

    blaze::addSpriteFrames(*spf.unwrap(), texture);

    BLAZE_TIMER_END();

    // Now test vanilla functions :)

    CCSpriteFrameCache::purgeSharedSpriteFrameCache();
    cache = CCSpriteFrameCache::get();

    {
        BLAZE_TIMER_START("Parse sprite frames (cocos)");

        auto dict = CCDictionary::createWithContentsOfFileThreadSafe(fp.c_str());
        if (!dict) {
            log::error("Error: failed to parse sprite frames: {}", spf.unwrapErr());
            return;
        }

        BLAZE_TIMER_STEP("Add sprite frames (cocos)");

        blaze::addSpriteFramesVanilla(dict, texture);

        dict->release();

        BLAZE_TIMER_END();
    }
}

static void bench() {
    benchSpriteFrames();
}

class $modify(MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto button = CCMenuItemExt::createSpriteExtra(
            CCSprite::create("fire.png"_spr),
            [] (auto) {
                bench();
            }
        );

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(button);
            menu->updateLayout();
        }

        return true;
    }
};
#endif

