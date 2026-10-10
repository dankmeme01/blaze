#include "GameModule.hpp"

using namespace geode::prelude;

namespace blaze {

GameModule::GameModule() {}

CCSpriteFrame* GameModule::spriteFrameForObjectId(int objectId) {
    if (!m_loaded.load(std::memory_order::acquire)) {
        // a worker may still be initializing the cache, it's unsafe to read right now. fall back.
        // this is rare, but can happen if the user opens a level very quickly after MenuLayer::init
        auto frame = ObjectToolbox::sharedState()->intKeyToFrame(objectId);
        return frame ? CCSpriteFrameCache::get()->spriteFrameByName(frame) : nullptr;
    }

    auto it = m_spriteFrameCache.find(objectId);
    if (it != m_spriteFrameCache.end()) {
        return it->second;
    }

    // try a safe fallback
    auto frame = ObjectToolbox::sharedState()->intKeyToFrame(objectId);
    return frame ? CCSpriteFrameCache::get()->spriteFrameByName(frame) : nullptr;
}

void GameModule::clearSpriteFrames() {
    m_loaded.store(false, std::memory_order::release);
    m_spriteFrameCache.clear();
}

// called in menulayer::init
void GameModule::populdateSpriteFrames() {
    if (m_loaded.load(std::memory_order::relaxed)) {
        // nothing to do!
        return;
    }

    // TODO 2.209: add new object ids
    auto ot = ObjectToolbox::sharedState();
    auto sfc = CCSpriteFrameCache::get();
    for (auto& [key, value] : ot->m_allKeys) {
        auto sf = sfc->spriteFrameByName(value.c_str());
        if (!sf) continue;

        m_spriteFrameCache.emplace(key, sf);
    }

    m_loaded.store(true, std::memory_order::release);
}

}

$on_game(TexturesUnloaded) {
    blaze::GameModule::get().clearSpriteFrames();
}
