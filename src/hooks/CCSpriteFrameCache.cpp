// Hook to make CCSpriteFrameCache functions a lot faster

#include "CCSpriteFrameCache.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCSpriteFrameCache.hpp>

#include "CCTextureCache.hpp"
#include <util/assert.hpp>
#include <util/hash.hpp>
#include <manager.hpp>
#include <TaskTimer.hpp>

using namespace geode::prelude;

namespace blaze {
    asp::Mutex<> g_sfcacheMutex;
    asp::Mutex<std::unordered_map<size_t, std::shared_ptr<SpriteFrameData>>> g_spfCache;

    std::shared_ptr<SpriteFrameData> loadSpriteFrames(const char* path) {
        BLAZE_ASSERT(path != nullptr);

        auto hash = blaze::hashStringRuntime(path);

        {
            auto cache = g_spfCache.lock();
            if (cache->contains(hash)) {
#ifdef BLAZE_DEBUG
                log::debug("loadSpriteFrames cache hit! for {}", path);
#endif
                return cache->at(hash);
            }
        }

#ifdef BLAZE_DEBUG
        log::debug("loadSpriteFrames cache miss for {}", path);
#endif

        size_t size;
        auto data = LoadManager::get().readFile(path, size, false);
        auto dataptr = data.release();

        auto result = blaze::parseSpriteFrames(dataptr, size, true);
        if (!result) {
            log::warn("Failed to parse sprite frames for {}: {}", path, result.unwrapErr());
            return nullptr;
        }

        std::shared_ptr<SpriteFrameData> spf = std::move(result).unwrap();
        g_spfCache.lock()->emplace(hash, spf);

        return spf;
    }
}

// Decided not to go for this hook as there hasn't been a significant enough improvement.

// class $modify(CCSpriteFrameCache) {
//     $override
//     void addSpriteFramesWithFile(const char* keyPtr) {
//         BLAZE_ASSERT_ALWAYS(keyPtr != nullptr);

//         BLAZE_TIMER_START("addSpriteFramesWithFile (pre checks)");

//         gd::string key = keyPtr;

//         {
//             auto _lck = blaze::g_sfcacheMutex.lock();
//             if (m_pLoadedFileNames->contains(key)) {
//                 return;
//             }
//         }

//         auto fu = CCFileUtils::get();

//         BLAZE_TIMER_STEP("addSpriteFramesWithFile (load)");

//         auto frames = blaze::loadSpriteFrames(keyPtr);

//         if (!frames) {
//             // Something failed, fallback to GD implementation
//             return CCSpriteFrameCache::addSpriteFramesWithFile(keyPtr);
//         }

//         BLAZE_TIMER_STEP("addSpriteFramesWithFile (texture load)");

//         // Obtain the PNG path
//         std::string texturePath;

//         if (frames->metadata.textureFileName && *frames->metadata.textureFileName) {
//             auto tfnsv = std::string_view(frames->metadata.textureFileName);

//             // we should strip the common parts here, for example
//             // if textureFileName is "a/b/sheet.png" and keyPtr is "a/b/sheet.plist",
//             // the tfnsv that we pass into the next function should be just "sheet.png"

//             // TODO: this is a lazy way to do it, but so far I don't think anything has been broken by this
//             tfnsv.remove_prefix(tfnsv.find_last_of('/') + 1);

//             texturePath = fu->fullPathFromRelativeFile(tfnsv.data(), keyPtr);
//         } else {
//             // build texture path by replacing file extension
//             texturePath = keyPtr;
//             texturePath.replace(texturePath.find(".plist"), 6, ".png");
//         }

//         auto texture = blaze::BTextureCache::get().loadTexture(texturePath.c_str());

//         BLAZE_TIMER_STEP("addSpriteFramesWithFile (adding sprite frames)");

//         if (texture) {
//             auto _lck = blaze::g_sfcacheMutex.lock();

//             blaze::addSpriteFrames(*frames, texture);
//             m_pLoadedFileNames->emplace(key);
//         } else {
//             log::warn("addSpriteFramesWithFile: failed to load texture at {}", texturePath);
//         }
//     }
// };
