#include <Geode/Geode.hpp>
#include <Geode/modify/CCSprite.hpp>
#include "EngineModule.hpp"

using namespace geode::prelude;

namespace blaze {

static CCGLProgram* g_spriteProgram;

static CCGLProgram* getSpriteProgram() {
    if (!g_spriteProgram) {
        g_spriteProgram = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    }

    return g_spriteProgram;
}

class $modify(CCSprite) {
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
        bool rotated
    ) {
        if (!CCNodeRGBA::init()) return false;

        m_pobBatchNode = nullptr;
        m_bRecursiveDirty = false;
        this->setDirty(false);

        m_bOpacityModifyRGB = true;

        m_sBlendFunc.src = CC_BLEND_SRC;
        m_sBlendFunc.dst = CC_BLEND_DST;

        m_bFlipX = m_bFlipY = false;

        this->setAnchorPoint({0.5f, 0.5f});

        // zwoptex default values
        m_obOffsetPosition = CCPoint{0.f, 0.f};

        m_bHasChildren = false;

        // clean the Quad
        m_sQuad = {};

        // Atlas: Color
        ccColor4B tmpColor = { 255, 255, 255, 255 };
        m_sQuad.bl.colors = tmpColor;
        m_sQuad.br.colors = tmpColor;
        m_sQuad.tl.colors = tmpColor;
        m_sQuad.tr.colors = tmpColor;

        // shader program
        this->setShaderProgram(getSpriteProgram());

        // update texture (calls updateBlendFunc)
        this->setTexture(texture);
        m_obUnflippedOffsetPositionFromCenter = offset;
        this->setTextureRect(rect, rotated, untrimmedSize);

        // by default use "Self Render".
        // if the sprite is added to a batchnode, then it will automatically switch to "batchnode Render"
        this->setBatchNode(nullptr);

        return true;
    }

    $override
    bool initWithTexture(CCTexture2D* texture, const CCRect& rect, bool rotated) {
        return this->initWithTextureEx(texture, rect, {}, rect.size, rotated);
    }

    $override
    bool initWithSpriteFrame(CCSpriteFrame* sf) {
        return this->initWithTextureEx(
            sf->getTexture(),
            sf->getRect(),
            sf->getOffset(),
            sf->getOriginalSize(),
            sf->isRotated()
        );
    }
};

}

$on_game(TexturesUnloaded) {
    blaze::g_spriteProgram = nullptr;
}
