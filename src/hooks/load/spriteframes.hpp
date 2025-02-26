#pragma once

#include <Geode/Result.hpp>
#include <cocos2d.h>

namespace blaze {

struct SpriteFrameData {
    struct Metadata {
        int format = -1;
    } metadata;
};

geode::Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size);

void addSpriteFrames(const SpriteFrameData& dict, cocos2d::CCTexture2D* texture);

}