#pragma once
#include <boost/container/vector.hpp>
#include <Geode/Result.hpp>

namespace blaze {

struct DecodedImage {
    std::unique_ptr<uint8_t[]> rawData;
    size_t rawSize;

    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    uint8_t channels;
    bool premultiplied;
};

geode::Result<boost::container::vector<uint8_t>> encodeFPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height);

geode::Result<DecodedImage> decodeSPNG(const uint8_t* data, size_t size);
geode::Result<DecodedImage> decodeFPNG(const uint8_t* data, size_t size);

}