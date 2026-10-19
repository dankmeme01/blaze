#include "CCSprite.hpp"
#include <util/assert.hpp>

using namespace geode::prelude;

namespace blaze {

static CCGLProgram* g_spriteProgram;

static CCGLProgram* getSpriteProgram() {
    if (!g_spriteProgram) {
        g_spriteProgram = CCShaderCache::sharedShaderCache()->programForKey(kCCShader_PositionTextureColor);
    }

    return g_spriteProgram;
}

bool CCSpriteExt::initWithTextureEx(
    CCTexture2D* texture,
    const CCRect& rect,
    const CCPoint& offset,
    const CCPoint& untrimmedSize,
    bool rotated,
    bool threadSafe
) {
    // NOLINTNEXTLINE(bugprone-parent-virtual-call)
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
    this->setShaderProgram(getSpriteProgram(), threadSafe);

    // update texture (calls updateBlendFunc)
    this->setTexture(texture, threadSafe);
    m_obUnflippedOffsetPositionFromCenter = offset;
    this->setTextureRect(rect, rotated, untrimmedSize);

    // by default use "Self Render".
    // if the sprite is added to a batchnode, then it will automatically switch to "batchnode Render"
    this->setBatchNode(nullptr);

    m_bDontDraw = false;

    return true;
}

static void atomicRetain(CCObject* obj) {
    std::atomic_ref<unsigned int> rc{obj->m_uReference};
    rc.fetch_add(1, std::memory_order::relaxed);
}

static void atomicRelease(CCObject* obj) {
    std::atomic_ref<unsigned int> rc{obj->m_uReference};

    if (rc.fetch_sub(1, std::memory_order::release) == 1) {
        std::atomic_thread_fence(std::memory_order::acquire);
        delete obj;
    }
}

template <typename T>
static void swapMember(T*& dst, T* src, bool atomic = false) {
    if (atomic) {
        atomicRetain(src);
        if (dst) {
            atomicRelease(dst);
        }
        dst = src;
    } else {
        src->retain();
        if (dst) {
            dst->release();
        }
        dst = src;
    }
}

void CCSpriteExt::setTexture(CCTexture2D* texture, bool atomic) {
    if (!atomic && !texture) {
        // no texture, grab a default one (only in the non-atomic path, atomic cannot do this)
        texture = CCTextureCache::get()->textureForKey("cc_2x2_white_image");
        if (!texture) {
            static uint8_t cc_2x2_white_image[] = {
                // RGBA8888
                0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF,
                0xFF, 0xFF, 0xFF, 0xFF
            };

            auto img = new CCImage();
            img->initWithImageData(cc_2x2_white_image, sizeof(cc_2x2_white_image), CCImage::kFmtRawData, 2, 2, 8);

            texture = CCTextureCache::get()->addUIImage(img, "cc_2x2_white_image");
            img->release();
        }
    }

    B_DEBUG_ASSERT(texture);

    if (m_pobBatchNode || m_pobTexture == texture) return;

    swapMember(m_pobTexture, texture, atomic);

    this->updateBlendFunc();
}

void CCSpriteExt::setShaderProgram(CCGLProgram* program, bool atomic) {
    swapMember(m_pShaderProgram, program, atomic);
}

bool CCSpriteExt::initWithTexture(CCTexture2D* texture, const CCRect& rect, bool rotated) {
    return this->initWithTextureEx(texture, rect, {}, rect.size, rotated);
}

bool CCSpriteExt::initWithSpriteFrame(CCSpriteFrame* sf) {
    return this->initWithTextureEx(
        sf->getTexture(),
        sf->getRect(),
        sf->getOffset(),
        sf->getOriginalSize(),
        sf->isRotated()
    );
}

bool CCSpriteExt::initWithSpriteFrameThreadSafe(CCSpriteFrame* sf) {
    return this->initWithTextureEx(
        sf->getTexture(),
        sf->getRect(),
        sf->getOffset(),
        sf->getOriginalSize(),
        sf->isRotated(),
        true
    );
}

Ref<CCSprite> CCSpriteExt::createFrameThreadSafe(const std::string& key) {
    auto obj = (CCSpriteFrame*)CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(key);
    if (!obj) return nullptr;

    auto ret = static_cast<CCSpriteExt*>(new CCSprite());
    if (!ret->initWithSpriteFrameThreadSafe(obj)) {
        ret->release();
        return nullptr;
    }

    return Ref<CCSprite>::adopt(ret);
}

}

$on_game(TexturesUnloaded) {
    blaze::g_spriteProgram = nullptr;
}
