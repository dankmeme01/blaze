#include "alpha.hpp"

#include <tracing.hpp>

#define RGB_PREMULTIPLY_ALPHA(r, g, b, a) \
    (((uint32_t)((r) * (a) / 255) << 0) | \
     ((uint32_t)((g) * (a) / 255) << 8) | \
     ((uint32_t)((b) * (a) / 255) << 16) | \
     (uint32_t)(a << 24))

namespace blaze {
    void premultiplyAlpha(void* dest, const void* source, size_t width, size_t height) {
        premultiplyAlpha(dest, source, width * height);
    }

    void premultiplyAlpha(void* dest, const void* source, size_t imageSize) {
        ZoneScoped;

        size_t iters = imageSize / sizeof(uint32_t);

        for (size_t i = 0; i < iters; i++) {
            const uint8_t* pixel = &(static_cast<const uint8_t*>(source))[4 * i];
            uint32_t* outpixel = &(static_cast<uint32_t*>(dest))[i];

            *outpixel = RGB_PREMULTIPLY_ALPHA(pixel[0], pixel[1], pixel[2], pixel[3]);
        }
    }
}