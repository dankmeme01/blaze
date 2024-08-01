#include "xor.hpp"
#include <asp/simd.hpp>
#include <util.hpp>

using xor_u8_impl_t = void (*)(uint8_t*, size_t, uint8_t);

static xor_u8_impl_t xor_u8_impl = nullptr;

void xor_u8_scalar(uint8_t* buffer, size_t size, uint8_t key) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] ^= key;
    }
}

#ifdef ASP_IS_X86

#include <immintrin.h>

void BLAZE_AVX2 xor_u8_avx2(uint8_t* buffer, size_t size, uint8_t key) {
    __m256i keyVec = _mm256_set1_epi8(key);

    // process 32 bytes at a time
    size_t i = 0;
    size_t vecSize = sizeof(__m256i);

    for (; i + vecSize <= size; i += vecSize) {
        // load data
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(buffer + i));
        // xor
        __m256i result = _mm256_xor_si256(data, keyVec);
        // sore
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(buffer + i), result);
    }

    // process remainder
    for (; i < size; i++) {
        buffer[i] ^= key;
    }
}

void BLAZE_SSE2 xor_u8_sse2(uint8_t* buffer, size_t size, uint8_t key) {
    __m128i keyVec = _mm_set1_epi8(key);

    // process 16 bytes at a time
    size_t i = 0;
    size_t vecSize = sizeof(__m128i);

    for (; i + vecSize <= size; i += vecSize) {
        // load data
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(buffer + i));
        // xor
        __m128i result = _mm_xor_si128(data, keyVec);
        // sore
        _mm_storeu_si128(reinterpret_cast<__m128i*>(buffer + i), result);
    }

    // process remainder
    for (; i < size; i++) {
        buffer[i] ^= key;
    }
}

#endif

static void chooseImpl() {
    auto& features = asp::simd::getFeatures();

#ifdef ASP_IS_X86
    if (features.avx2) {
        xor_u8_impl = &xor_u8_avx2;
    } else if (features.avx) {
        xor_u8_impl = &xor_u8_sse2;
    } else {
        xor_u8_impl = &xor_u8_scalar;
    }
#elif defined(ASP_IS_ARM64)
    xor_u8_impl = &xor_u8_neon;
#else
    xor_u8_impl = &xor_u8_scalar;
#endif
}

void blaze::xor_u8(uint8_t* buffer, size_t size, uint8_t key) {
    if (!xor_u8_impl) chooseImpl();

    xor_u8_impl(buffer, size, key);
}
