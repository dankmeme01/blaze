#include "alpha.hpp"

#include <asp/simd.hpp>
#include <util.hpp>

#ifdef ASP_IS_X86
# include <immintrin.h>
#endif

#include <tracing.hpp>

// SIMD versions are taken from
// https://github.com/Wizermil/premultiply_alpha/blob/master/premultiply_alpha/premultiply_alpha.hpp
// and slightly modified to make them work for unaligned data

#define SCALAR_PREMUL_PIXEL(r, g, b, a) \
    (((uint32_t)((r) * (a) / 255) << 0) | \
     ((uint32_t)((g) * (a) / 255) << 8) | \
     ((uint32_t)((b) * (a) / 255) << 16) | \
     (uint32_t)(a << 24))

namespace blaze {
    using premul_inplace_impl_t = void (*)(void*, size_t);
    static premul_inplace_impl_t premul_inplace_impl = nullptr;

    static void premultiplyAlphaScalar(void* dest, const void* source, size_t imageSize) {
        size_t iters = imageSize / sizeof(uint32_t);

        for (size_t i = 0; i < iters; i++) {
            const uint8_t* pixel = &(static_cast<const uint8_t*>(source))[4 * i];
            uint32_t* outpixel = &(static_cast<uint32_t*>(dest))[i];

            *outpixel = SCALAR_PREMUL_PIXEL(pixel[0], pixel[1], pixel[2], pixel[3]);
        }
    }

    static void premultiplyAlphaScalarInplace(void* data, size_t imageSize) {
        size_t iters = imageSize / sizeof(uint32_t);

        for (size_t i = 0; i < iters; i++) {
            const uint8_t* pixel = &(static_cast<const uint8_t*>(data))[4 * i];
            uint32_t* outpixel = &(static_cast<uint32_t*>(data))[i];

            *outpixel = SCALAR_PREMUL_PIXEL(pixel[0], pixel[1], pixel[2], pixel[3]);
        }
    }

    // Simd algorithms are only available for in-place
    // If this will need to be changed in the future, they will need to be adjusted a bit.

#ifdef ASP_IS_X86
    static BLAZE_SSE2 void premultiplyAlphaSSE2(void* data, size_t imageSize) {

    }

    static BLAZE_SSSE3 void premultiplyAlphaSSSE3(void* data, size_t imageSize) {
        size_t const max_simd_pixel = imageSize / sizeof(__m128i) * sizeof(__m128i);

        __m128i const mask_alphha_color_odd_255 = _mm_set1_epi32(static_cast<int>(0xff000000));
        __m128i const div_255 = _mm_set1_epi16(static_cast<short>(0x8081));

        __m128i const mask_shuffle_alpha = _mm_set_epi32(0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380);
        __m128i const mask_shuffle_color_odd = _mm_set_epi32(static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180));

        __m128i color, alpha, color_even, color_odd;
        for (size_t i = 0; i < max_simd_pixel; i += sizeof(__m128i)) {
            __m128i* ptr = reinterpret_cast<__m128i*>(reinterpret_cast<uint8_t*>(data) + i);

            color = _mm_loadu_si128(ptr);

            alpha = _mm_shuffle_epi8(color, mask_shuffle_alpha);

            color_even = _mm_slli_epi16(color, 8);
            color_odd = _mm_shuffle_epi8(color, mask_shuffle_color_odd);
            color_odd = _mm_or_si128(color_odd, mask_alphha_color_odd_255);
//            color_odd = _mm_blendv_epi8(color, _mm_set_epi32(0xff000000, 0xff000000, 0xff000000, 0xff000000), _mm_set_epi32(0x80800080, 0x80800080, 0x80800080, 0x80800080));

            color_odd = _mm_mulhi_epu16(color_odd, alpha);
            color_even = _mm_mulhi_epu16(color_even, alpha);

            color_odd = _mm_srli_epi16(_mm_mulhi_epu16(color_odd, div_255), 7);
            color_even = _mm_srli_epi16(_mm_mulhi_epu16(color_even, div_255), 7);

            color = _mm_or_si128(color_even, _mm_slli_epi16(color_odd, 8));

            _mm_storeu_si128(ptr, color);
        }

        size_t const remaining_pixel = imageSize - max_simd_pixel;
        premultiplyAlphaScalarInplace(reinterpret_cast<uint8_t*>(data) + max_simd_pixel, remaining_pixel);
    }

    static BLAZE_AVX2 void premultiplyAlphaAVX2(void* data, size_t imageSize) {
        size_t const max_simd_pixel = imageSize / sizeof(__m256i) * sizeof(__m256i);

        __m256i const mask_alphha_color_odd_255 = _mm256_set1_epi32(static_cast<int>(0xff000000));
        __m256i const div_255 = _mm256_set1_epi16(static_cast<short>(0x8081));

        __m256i const mask_shuffle_alpha = _mm256_set_epi32(0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380, 0x0f800f80, 0x0b800b80, 0x07800780, 0x03800380);
        __m256i const mask_shuffle_color_odd = _mm256_set_epi32(static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180), static_cast<int>(0x80800d80), static_cast<int>(0x80800980), static_cast<int>(0x80800580), static_cast<int>(0x80800180));

        __m256i color, alpha, color_even, color_odd;
        for (size_t i = 0; i < max_simd_pixel; i += sizeof(__m256i)) {
            __m256i* ptr = reinterpret_cast<__m256i*>(reinterpret_cast<uint8_t*>(data) + i);
            color = _mm256_loadu_si256(ptr);

            alpha = _mm256_shuffle_epi8(color, mask_shuffle_alpha);

            color_even = _mm256_slli_epi16(color, 8);
            color_odd = _mm256_shuffle_epi8(color, mask_shuffle_color_odd);
            color_odd = _mm256_or_si256(color_odd, mask_alphha_color_odd_255);

            color_odd = _mm256_mulhi_epu16(color_odd, alpha);
            color_even = _mm256_mulhi_epu16(color_even, alpha);

            color_odd = _mm256_srli_epi16(_mm256_mulhi_epu16(color_odd, div_255), 7);
            color_even = _mm256_srli_epi16(_mm256_mulhi_epu16(color_even, div_255), 7);

            color = _mm256_or_si256(color_even, _mm256_slli_epi16(color_odd, 8));

            _mm256_storeu_si256(ptr, color);
        }

        size_t const remaining_pixel = imageSize - max_simd_pixel;
        premultiplyAlphaScalarInplace(reinterpret_cast<uint8_t*>(data) + max_simd_pixel, remaining_pixel);
    }
#endif

    static void chooseImpl() {
        auto& features = asp::simd::getFeatures();
        premul_inplace_impl = &premultiplyAlphaScalarInplace;

#ifdef ASP_IS_X86
        if (features.avx2) {
            premul_inplace_impl = &premultiplyAlphaAVX2;
        } else if (features.ssse3) {
            premul_inplace_impl = &premultiplyAlphaSSSE3;
        }
// #elif defined (ASP_IS_ARM64)
//         premul_inplace_impl = &premultiplyAlphaNEON;
#endif
    }

    void premultiplyAlpha(void* dest, const void* source, size_t width, size_t height) {
        premultiplyAlpha(dest, source, width * height);
    }

    void premultiplyAlpha(void* dest, const void* source, size_t imageSize) {
        ZoneScoped;

        premultiplyAlphaScalar(dest, source, imageSize);
    }

    void premultiplyAlphaInplace(void* data, size_t width, size_t height) {
        return premultiplyAlphaInplace(data, width * height);
    }

    void premultiplyAlphaInplace(void* data, size_t imageSize) {
        ZoneScoped;

        if (!premul_inplace_impl) chooseImpl();

        premul_inplace_impl(data, imageSize);
    }
}