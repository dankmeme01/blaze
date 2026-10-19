#include "GameModule.hpp"

using namespace geode::prelude;

namespace blaze {

static std::string findFrame(int objectId) {
    static auto ot = ObjectToolbox::sharedState();
    auto& map = ot->m_allKeys;
    auto it = map.find(objectId);
    if (it != map.end()) {
        return it->second;
    }
    return "";
}

GameModule::GameModule() {}

CCSpriteFrame* GameModule::spriteFrameForObjectId(int objectId) {
    if (!m_loaded.load(std::memory_order::acquire)) {
        // a worker may still be initializing the cache, it's unsafe to read right now. fall back.
        // this is rare, but can happen if the user opens a level very quickly after MenuLayer::init
        auto frame = findFrame(objectId);
        return frame.empty() ? nullptr : CCSpriteFrameCache::get()->spriteFrameByName(frame.c_str());
    }

    auto it = m_spriteFrameCache.find(objectId);
    if (it != m_spriteFrameCache.end()) {
        return it->second;
    }

    // try a safe fallback
    auto frame = findFrame(objectId);
    return frame.empty() ? nullptr : CCSpriteFrameCache::get()->spriteFrameByName(frame.c_str());
}

std::string GameModule::frameNameForObjectId(int objectId) {
    if (m_loaded.load(std::memory_order::acquire)) {
        auto it = m_frameNameCache.find(objectId);
        if (it != m_frameNameCache.end()) {
            return it->second;
        }
    }

    // worker still initializing or not found in cache, try a safe fallback
    return findFrame(objectId);
}

void GameModule::clearSpriteFrames() {
    m_loaded.store(false, std::memory_order::release);
    m_spriteFrameCache.clear();
    m_frameNameCache.clear();
}

// called in menulayer::init
void GameModule::populateSpriteFrames() {
    if (m_loaded.load(std::memory_order::relaxed)) {
        // nothing to do!
        return;
    }

    // TODO 2.209: add new object ids
    auto ot = ObjectToolbox::sharedState();
    auto sfc = CCSpriteFrameCache::get();
    for (auto& [key, value] : ot->m_allKeys) {
        m_frameNameCache.emplace(key, value);

        auto sf = (CCSpriteFrame*) sfc->m_pSpriteFrames->objectForKey(value);
        if (!sf) continue;

        m_spriteFrameCache.emplace(key, sf);
    }

    m_loaded.store(true, std::memory_order::release);
}

}

$on_game(TexturesUnloaded) {
    blaze::GameModule::get().clearSpriteFrames();
}
