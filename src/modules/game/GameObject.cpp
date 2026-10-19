#include "GameModule.hpp"
#include <Geode/modify/GameObject.hpp>
#include <modules/engine/CCSprite.hpp>
#include "object/objects.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(GameObject) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("GameObject::addCustomChild", Priority::Replace);
        (void) self.setHookPriority("GameObject::addCustomColorChild", Priority::Replace);
        (void) self.setHookPriority("GameObject::addInternalGlowChild", Priority::Replace);
        (void) self.setHookPriority("GameObject::addInternalCustomColorChild", Priority::Replace);
    }

    CCSprite* addCustomChild(std::string frame, CCPoint offset, int zOrder) {
        m_hasCustomChild = true;
        auto origin = convertToNodeSpace({});
        auto s = CCSpriteExt::createFrameThreadSafe(frame);
        s->setPosition(origin + offset);
        this->addChild(s, zOrder);
        return s;
    }

    CCSprite* addCustomColorChild(std::string frame) {
        if (auto spr = std::exchange(m_colorSprite, nullptr)) {
            spr->removeFromParent();
            spr->release();
        }

        if (!m_detailColor) {
            m_detailColor = new GJSpriteColor();
            m_detailColor->m_defaultColorID = 1;
            m_detailColor->m_colorID = 0;
        }

        auto spr = CCSpriteExt::createFrameThreadSafe(frame);
        spr->setPosition(getPosition());
        m_colorSprite = spr.take();
        return m_colorSprite;
    }

    CCSprite* addInternalCustomColorChild(std::string frame, CCPoint offset, int zOrder) {
        if (!m_colorSprite) return nullptr;
        auto origin = m_colorSprite->convertToNodeSpace({});

        auto spr = CCSpriteExt::createFrameThreadSafe(frame);
        spr->setPosition(origin + offset);
        m_colorSprite->addChild(spr, zOrder);
        return spr;
    }

    CCSprite* addInternalGlowChild(std::string frame, CCPoint offset) {
        if (!m_glowSprite) return nullptr;
        auto origin = m_glowSprite->convertToNodeSpace({});

        auto spr = CCSpriteExt::createFrameThreadSafe(frame);
        spr->setPosition(origin + offset);
        m_glowSprite->addChild(spr, -1);
        return spr;
    }
};

}
