#include "CCTextureCache.hpp"

#include <asp/sync/Mutex.hpp>
#include <Geode/loader/Log.hpp>
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

CCTexture2D* BTextureCache::loadTexture(const char* path) {
    blaze::ThreadSafeFileUtilsGuard _guard;

    auto fullPath = blaze::fullPathForFilename(path, false);
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

    // convert extension to lowercase
    std::string extension(extensionPre.size(), '\0');
    std::transform(extensionPre.begin(), extensionPre.end(), extension.begin(), ::tolower);

    if (extension != ".png") {
#ifdef BLAZE_DEBUG
        log::warn("Non-PNG image is being loaded, using builtin handler ({})", path);
#endif
        return CCTextureCache::addImage(path, false);
    }

    size_t size = 0;
    auto buffer = LoadManager::get().readFile(fullPath.c_str(), size, true);

    if (!buffer || size == 0) {
        log::warn("Failed to load image at path {}, using builtin handler", fullPath);
        return CCTextureCache::addImage(path, false);
    }

    auto image = new CCImage();
    auto res = static_cast<CCImageExt*>(image)->initWithSPNGOrCache(buffer.get(), size, fullPath.data());
    if (!res) {
        log::warn("Failed to load image at path {}, using builtin handler: {}", fullPath, res.unwrapErr());
        return CCTextureCache::addImage(path, false);
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

} // namespace blaze