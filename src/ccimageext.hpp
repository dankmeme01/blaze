#pragma once
#include <cocos2d.h>
#include <Geode/utils/Result.hpp>

#include <formats.hpp>
#include <util.hpp>

namespace blaze {

class CCImageExt : public cocos2d::CCImage {
public:
    uint8_t* getImageData();

    void setImageData(uint8_t* buf);

    void setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti);

    geode::Result<> initWithSPNG(const void* data, size_t size);
    geode::Result<> initWithFPNG(const void* data, size_t size);
    geode::Result<> initWithSPNGOrCache(const uint8_t* data, size_t size, const char* imgPath);
    geode::Result<> initWithSPNGOrCache(const blaze::OwnedMemoryChunk& chunk, const char* imgPath);

private:
    void initWithDecodedImage(DecodedImage&);
};

}