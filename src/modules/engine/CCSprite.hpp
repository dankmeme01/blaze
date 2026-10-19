#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include "EngineModule.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(CCSpriteExt, CCSprite) {
    static void onModify(auto& self) {
        EngineModule::get().addHooks(self);

        (void) self.setHookPriority("cocos2d::CCSprite::initWithTexture", Priority::Replace);
        (void) self.setHookPriority("cocos2d::CCSprite::initWithSpriteFrame", Priority::Replace);
    }

    bool initWithTextureEx(
        CCTexture2D* texture,
        const CCRect& rect,
        const CCPoint& offset,
        const CCPoint& untrimmedSize,
        bool rotated,
        bool threadSafe = false
    );

    $override
    bool initWithTexture(CCTexture2D* texture, const CCRect& rect, bool rotated);

    $override
    bool initWithSpriteFrame(CCSpriteFrame* sf);

    /// like `initWithSpriteFrame`, but this is thread safe in terms of the refcount of the previous & new `CCTexture2D`,
    /// and only if no one else touches those textures without using this thread-safe method in the meantime.
    bool initWithSpriteFrameThreadSafe(CCSpriteFrame* sf);

    /// Creates a CCSprite but does not autorelease and does not use Geode's fallback, being essentially thread safe if sfcache is not mutated
    static Ref<CCSprite> createFrameThreadSafe(const std::string& key);

    void setTexture(CCTexture2D* texture, bool atomic = false);
    void setShaderProgram(CCGLProgram* program, bool atomic = false);
};


}
