#pragma once
#include <stddef.h>

namespace blaze {
    // Premultiply alpha using the raw image data from `source` and store it in `dest`.
    void premultiplyAlpha(void* dest, const void* source, size_t width, size_t height);

    // Premultiply alpha using the raw image data from `source` and store it in `dest`.
    void premultiplyAlpha(void* dest, const void* source, size_t imageSize);
}