#pragma once

#include <Geode/Geode.hpp>
#include <util/macros.hpp>

// Thread safe object creation functions, not using autorelease

namespace blaze {

geode::Ref<GameObject> createGameObject(int objectId, cocos2d::CCSpriteFrame* frame, bool* needsCustomSetup = nullptr);

int& getUniqueGameObjectId();
uint64_t& getVarianceSeed();

inline short nextVarianceValue(uint64_t& s) {
    s = s * 0x343fd + 0x269ec3;
    float a = static_cast<float>((s >> 16) & 0x7fff) / 32767.f;
    return static_cast<short>(a * 1900.f);
}

}
