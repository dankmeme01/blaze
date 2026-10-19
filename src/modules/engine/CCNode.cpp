#include <Geode/Geode.hpp>
#include <Geode/modify/CCNode.hpp>
#include "CCArray.hpp"
#include "EngineModule.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(CCArrayExt, CCNode) {
    static void onModify(auto& self) {
        EngineModule::get().addHooks(self);

        (void) self.setHookPriority("cocos2d::CCNode::addChild", Priority::Replace * 1000);
    }

    void addChild(CCNode* child, int zOrder, int tag) {
        if (!m_pChildren) {
            m_pChildren = ArrayExt::createWithCapacity(4).take();
        }

        CCNode::addChild(child, zOrder, tag);
    }
};

}

