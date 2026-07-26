#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextureCache.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

namespace blaze {

struct HookedTextureCache : Modify<HookedTextureCache, CCTextureCache> {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
#ifdef BLAZE_DEBUG
            "cocos2d::CCTextureCache::addImage"
#endif
        );
    }

#ifdef BLAZE_DEBUG
    $override
    CCTexture2D* addImage(const char* path, bool ignoreSuffix) {
        auto start = asp::Instant::now();
        auto tex = CCTextureCache::addImage(path, ignoreSuffix);
        auto taken = start.elapsed();

        if (taken.millis() > 0) {
            log::debug("CCTextureCache::addImage({}) took {}", path, taken);
        }

        return tex;
    }

#endif
};

}
