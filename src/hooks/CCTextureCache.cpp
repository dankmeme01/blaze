#include "CCTextureCache.hpp"

#include <asp/sync/Mutex.hpp>
#include <Geode/loader/Log.hpp>
#include <Geode/modify/CCTextureCache.hpp>
#include <ccimageext.hpp>
#include <manager.hpp>
#include <fpff.hpp>

using namespace geode::prelude;

static asp::Mutex<> s_texturesMutex;

namespace blaze {

BTextureCache& BTextureCache::get() {
    return static_cast<BTextureCache&>(*CCTextureCache::get());
}

void BTextureCache::removeTexture(const gd::string& key) {
    auto _lck = s_texturesMutex.lock();
    this->m_pTextures->removeObjectForKey(key);
}

void BTextureCache::setTexture(const gd::string& key, CCTexture2D* texture) {
    auto _lck = s_texturesMutex.lock();
    this->m_pTextures->setObject(texture, key);
}

static bool equalIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (tolower(a[i]) != tolower(b[i])) {
            return false;
        }
    }

    return true;
}

CCTexture2D* BTextureCache::loadTexture(const char* path, bool ignoreSuffix) {
    blaze::ThreadSafeFileUtilsGuard _guard;

    auto fullPath = blaze::fullPathForFilename(path, ignoreSuffix);
    if (fullPath.empty()) {
        return nullptr;
    }

    std::string_view fullPathsv = std::string_view(fullPath.data(), fullPath.size());

    {
        auto _lck = s_texturesMutex.lock();
        if (auto tex = this->m_pTextures->objectForKey(fullPath)) {
            return static_cast<CCTexture2D*>(tex);
        }
    }

    std::string_view extensionPre = fullPathsv.substr(fullPathsv.find_last_of('.'));
    bool isPng = equalIgnoreCase(extensionPre, ".png");

    if (!isPng) {
#ifdef BLAZE_DEBUG
        log::warn("Non-PNG image is being loaded, using builtin handler ({})", path);
#endif
        return CCTextureCache::addImage(path, ignoreSuffix);
    }

    size_t size = 0;
    auto buffer = LoadManager::get().readFile(fullPath.c_str(), size, true);

    if (!buffer || size == 0) {
        // log::debug("Failed to read image at path {}, returning null", fullPath);
        // return CCTextureCache::addImage(path, false);
        return nullptr;
    }

    auto image = new CCImage();
    auto res = static_cast<CCImageExt*>(image)->initWithSPNGOrCache(buffer.get(), size, fullPath.data());
    if (!res) {
        log::warn("Failed to decode image at path {}, returning null: {}", fullPath, res.unwrapErr());
        // return CCTextureCache::addImage(path, false);
        image->release();
        return nullptr;
    }

    auto texture = new CCTexture2D();
    if (!texture->initWithImage(image)) {
        log::warn("Texture init failed, this is very bad!");
        log::warn("Path: {}", fullPath);
        texture->release();
        image->release();
        return nullptr;
    }

    auto _lck = s_texturesMutex.lock();
    this->m_pTextures->setObject(texture, path);

    texture->release();
    image->release();

    return texture;
}

struct CCTextureCacheHook : Modify<CCTextureCacheHook, CCTextureCache> {
    $override
    CCTexture2D* addImage(const char* path, bool ignoreSuffix) {
        return BTextureCache::get().loadTexture(path, ignoreSuffix);
    }
};

} // namespace blaze