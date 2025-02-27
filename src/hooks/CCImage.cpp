// Blaze CCImage hooks.
//
// Intercepts `initWithImageFile` and `initWithImageFileThreadSafe` to use an image cache and use the SPNG decoder.
// Intercepts `_initWithPngData` to use the SPNG decoder.
//
// All images are cached, even ones from other mods.

#include <Geode/Geode.hpp>
#include <Geode/modify/CCImage.hpp>

#include <asp/data.hpp>

#include <manager.hpp>
#include <formats.hpp>
#include <ccimageext.hpp>

using namespace geode::prelude;
using blaze::CCImageExt;

class $modify(CCImage) {
    CCImageExt* ext() {
        return static_cast<CCImageExt*>(static_cast<CCImage*>(this));
    }

    static void onModify(auto& self) {
        // run last since we dont call the originals
        (void) self.setHookPriority("cocos2d::CCImage::initWithImageFile", 9999999).unwrap();
        (void) self.setHookPriority("cocos2d::CCImage::initWithImageFileThreadSafe", 9999999).unwrap();

#ifdef GEODE_IS_MACOS // Mac does not have _initWithPngData
        (void) self.setHookPriority("cocos2d::CCImage::initWithImageData", 9999999).unwrap();
#else
        (void) self.setHookPriority("cocos2d::CCImage::_initWithPngData", 9999999).unwrap();
#endif
    }

    bool initWithImageFileCommon(uint8_t* buffer, size_t size, EImageFormat format, const char* path) {
        // if not png, just do the default impl of this func
        if (format != CCImage::EImageFormat::kFmtPng && !std::string_view(path).ends_with(".png")) {
            return CCImage::initWithImageData(buffer, size);
        }

        auto res = this->ext()->initWithSPNGOrCache(buffer, size, path);
        if (res) {
            return true;
        } else {
            log::warn("Failed to load image ({}) at path {}", res.unwrapErr(), path);
            return false;
        }
    }

    $override
    bool initWithImageFile(const char* path, EImageFormat format) {
        size_t size = 0;

        auto buffer = LoadManager::get().readFile(path, size);

        if (!buffer || size == 0) {
            return false;
        }

        return this->initWithImageFileCommon(buffer.get(), size, format, path);
    }

    $override
    bool initWithImageFileThreadSafe(const char* path, EImageFormat format) {
        return this->initWithImageFile(path, format); // who cares about thread safety?
    }

#ifndef GEODE_IS_MACOS
    $override
    bool initWithImageData(void* data, int size, EImageFormat format, int a, int b, int c, int d) {
        // If this is a PNG image, use spng
        if (format != CCImage::EImageFormat::kFmtPng) {
            return CCImage::initWithImageData(data, size, format, a, b, c, d);
        }

        auto res = this->ext()->initWithSPNG(data, size);
        if (res) {
            return true;
        } else {
            log::warn("CCImage hook: initWithSPNG failed", res.unwrapErr());
            return false;
        }
    }
#else
    $override
    bool _initWithPngData(void* data, int size) {
        auto res = this->ext()->initWithSPNG(data, size);
        if (!res) {
            log::warn("CCImage hook: initWithSPNG failed", res.unwrapErr());
            return false;
        }

        return true;
    }
#endif
};
