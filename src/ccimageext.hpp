#pragma once
#include <cocos2d.h>
#include <Geode/utils/Result.hpp>

#include <formats.hpp>

namespace blaze {

class CCImageExt : public cocos2d::CCImage {
public:
    uint8_t* getImageData();

    void setImageData(uint8_t* buf);

    void setImageProperties(uint32_t width, uint32_t height, uint32_t bitDepth, bool hasAlpha, bool hasPreMulti);

    geode::Result<> initWithSPNG(void* data, size_t size);
    geode::Result<> initWithFPNG(void* data, size_t size);
    geode::Result<> initWithSPNGOrCache(uint8_t* data, size_t size, const char* imgPath);

private:
    void initWithDecodedImage(DecodedImage&);
};

}