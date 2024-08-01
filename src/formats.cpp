#include "formats.hpp"

#include <spng.h>
#include <fpng.h>
#include <algo/alpha.hpp>
#include <tracing.hpp>

using namespace geode::prelude;

namespace blaze {

Result<boost::container::vector<uint8_t>> encodeFPNG(const uint8_t* data, size_t size, uint32_t width, uint32_t height) {
    ZoneScoped;

    boost::container::vector<uint8_t> out;

    size_t channels = size / (width * height);
    if (!fpng::fpng_encode_image_to_memory(
        data,
        width, height,
        channels,
        out,
        0
    )) {
        return Err(fmt::format("FPNG encode failed! (img data: {}x{}, {} channels, {} bytes)", width, height, channels, size));
    }

    return Ok(std::move(out));
}


#define SPNG_EC(fun) if (auto _ec = (fun) != 0) { \
    spng_ctx_free(ctx); \
	return Err(fmt::format("{} failed: {}", #fun, spng_strerror(_ec))); \
}

Result<DecodedImage> decodeSPNG(const uint8_t* data, size_t size) {
    ZoneScoped;

    DecodedImage image;

    spng_ctx* ctx = spng_ctx_new(0);

    if (!ctx) {
        return Err("failed to allocate spng context");
    }

    SPNG_EC(spng_set_png_buffer(ctx, data, size));

    spng_ihdr hdr;
    SPNG_EC(spng_get_ihdr(ctx, &hdr));

    image.width = hdr.width;
    image.height = hdr.height;
    image.bitDepth = hdr.bit_depth;
    image.premultiplied = false;

    SPNG_EC(spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image.rawSize));

    image.channels = image.rawSize / (image.width * image.height);

    image.rawData = std::make_unique<uint8_t[]>(image.rawSize);

    SPNG_EC(spng_decode_image(ctx, image.rawData.get(), image.rawSize, SPNG_FMT_RGBA8, SPNG_DECODE_TRNS));

    spng_ctx_free(ctx);

    return Ok(std::move(image));
}

Result<DecodedImage> decodeFPNG(const uint8_t* data, size_t size) {
    ZoneScoped;

    DecodedImage image;

    uint32_t channels;

    uint8_t* rawData;
    size_t rawSize;

    if (auto code = fpng::fpng_decode_memory_ptr(data, size, rawData, rawSize, image.width, image.height, channels, 4)) {
        return Err(fmt::format("fpng_decode_memory failed: code {}", code));
    }

    image.rawData = std::unique_ptr<uint8_t[]>(rawData);
    image.rawSize = rawSize;
    image.channels = channels;
    image.bitDepth = 8;
    image.premultiplied = false;

    return Ok(std::move(image));
}

}
