#pragma once
#include <modules/Module.hpp>
#include <gtl/phmap.hpp>

namespace blaze {

class GameModule : public Module<GameModule> {
public:
    GameModule();

    cocos2d::CCSpriteFrame* spriteFrameForObjectId(int objectId);
    cocos2d::CCSpriteFrame* spriteFrameForName(std::string_view name);
    std::string frameNameForObjectId(int objectId);

    void clearSpriteFrames();
    void populateSpriteFrames();

private:
    std::atomic<bool> m_loaded{false};
    gtl::flat_hash_map<int, cocos2d::CCSpriteFrame*> m_spriteFrameCache;
    gtl::flat_hash_map<int, std::string> m_frameNameCache;
};

}
