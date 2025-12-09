#pragma once

#include <cocos2d.h>

namespace blaze {
    // Thread-safe wrapper around `CCTextureCache`.
    class BTextureCache : public cocos2d::CCTextureCache {
    public:
        static BTextureCache& get();

        // Calls `m_pTextures->removeObjectForKey` in a thread-safe manner
        void removeTexture(const gd::string& key);

        // Calls `m_pTextures->setObject` in a thread-safe manner
        void setTexture(const gd::string& key, cocos2d::CCTexture2D* texture);

        // Optimized implementation of `addImage`, thread-safe only if the texture has already been laoded.
        cocos2d::CCTexture2D* loadTexture(const char* path);
    };
} // namespace blaze

