#include "ccimageext.hpp"

#include <asp/data.hpp>
#include <fpng.h>
#include <spng.h>

#include <manager.hpp>
#include <algo/alpha.hpp>
#include <algo/crc32.hpp>
#include <tracing.hpp>
#include <settings.hpp>
#include <fpff.hpp>

#include <Geode/loader/Log.hpp>
#include <Geode/Prelude.hpp>

using namespace geode::prelude;

namespace blaze {

uint8_t* CCImageExt::getImageData() {
    return m_pData;
}

void CCImageExt::setImageData(uint8_t* buf) {
    delete[] m_pData;
    m_pData = buf;
}

void CCImageExt::setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti) {
    this->m_nWidth = width;
    this->m_nHeight = height;
    this->m_nBitsPerComponent = bitDepth;
    this->m_bHasAlpha = hasAlpha;
    this->m_bPreMulti = hasPreMulti;
}

void CCImageExt::initWithDecodedImage(DecodedImage& img) {
    ZoneScoped;

    m_nWidth = img.width;
    m_nHeight = img.height;
    m_bHasAlpha = img.channels == 4;
    m_bPreMulti = m_bHasAlpha;
    m_pData = img.rawData.release();

    // if the image is transparent and not premultiplied, premultiply it
    if (m_bHasAlpha && !img.premultiplied) {
        premultiplyAlphaInplace(m_pData, img.rawSize);
    }
}

Result<> CCImageExt::initWithSPNG(const void* data, size_t size) {
    GEODE_UNWRAP_INTO(auto img, blaze::decodeSPNG(static_cast<const uint8_t*>(data), size));

    this->initWithDecodedImage(img);

    return Ok();
}

Result<> CCImageExt::initWithFPNG(const void* data, size_t size) {
    GEODE_UNWRAP_INTO(auto img, blaze::decodeFPNG(static_cast<const uint8_t*>(data), size));

    this->initWithDecodedImage(img);

    return Ok();
}

Result<> CCImageExt::initWithSPNGOrCache(const uint8_t* buffer, size_t size, const char* imgPath) {
    ZoneScoped;

    // if image cache is disabled, or small mode is enabled and image is <64k, don't do caching
    if (!blaze::settings().imageCache || (blaze::settings().imageCacheSmall && size < 65536)) {
        return this->initWithSPNG(buffer, size);
    }

    blaze::ThreadSafeFileUtilsGuard _guard;

    // check if we have cached it already
    auto checksum = blaze::crc32(buffer, size);
    uint8_t* cachedBuf = nullptr;
    size_t cachedSize = 0;

    if (!LoadManager::get().reallocFromCache(checksum, cachedBuf, cachedSize)) {
        // queue the image to be cached..
        std::vector<uint8_t> data(buffer, buffer + size);

        LoadManager::get().queueForCache(std::filesystem::path(blaze::fullPathForFilename(imgPath)), std::move(data));
        return this->initWithSPNG(buffer, size);
    }

    auto result = this->initWithFPNG(cachedBuf, cachedSize);
    delete[] cachedBuf;

    if (result) {
        return Ok();
    }

    auto error = std::move(result.unwrapErr());

    // god i hate gd::string
#ifdef GEODE_IS_ANDROID
    std::filesystem::path p{std::string(blaze::fullPathForFilename(imgPath))};
#else
    std::filesystem::path p = blaze::fullPathForFilename(imgPath);
#endif

    auto cachedFile = LoadManager::get().cacheFileForChecksum(checksum);

    log::warn("Failed to load cached image: {}", error);
    log::warn("Real: {}, cached: {}", p, cachedFile);

    std::error_code ec;
    std::filesystem::remove(cachedFile, ec);

    // LoadManager::get().queueForCache(p, {});

    return this->initWithSPNG(buffer, size);
}

Result<> CCImageExt::initWithSPNGOrCache(const blaze::OwnedMemoryChunk& chunk, const char* imgPath) {
    return this->initWithSPNGOrCache(chunk.data, chunk.size, imgPath);
}

}