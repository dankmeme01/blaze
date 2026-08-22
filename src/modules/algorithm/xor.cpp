#include "xor.hpp"
#include <util/simd.hpp>

using xor_u8_impl_t = void (*)(uint8_t*, uint8_t*, size_t, uint8_t);

static xor_u8_impl_t xor_u8_impl = nullptr;

void xor_u8_scalar(uint8_t* src, uint8_t* dst, size_t size, uint8_t key) {
    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i] ^ key;
    }
}

#ifdef ASP_IS_X86

#include <immintrin.h>

void BLAZE_AVX2 xor_u8_avx2(uint8_t* src, uint8_t* dst, size_t size, uint8_t key) {
    __m256i keyVec = _mm256_set1_epi8((char)key);

    // process 32 bytes at a time
    size_t i = 0;
    size_t vecSize = sizeof(__m256i);

    for (; i + vecSize <= size; i += vecSize) {
        // load data
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(src + i));
        // xor
        __m256i result = _mm256_xor_si256(data, keyVec);
        // store
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dst + i), result);
    }

    // process remainder
    for (; i < size; i++) {
        dst[i] = src[i] ^ key;
    }
}

void BLAZE_SSE2 xor_u8_sse2(uint8_t* src, uint8_t* dst, size_t size, uint8_t key) {
    __m128i keyVec = _mm_set1_epi8((char)key);

    // process 16 bytes at a time
    size_t i = 0;
    size_t vecSize = sizeof(__m128i);

    for (; i + vecSize <= size; i += vecSize) {
        // load data
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + i));
        // xor
        __m128i result = _mm_xor_si128(data, keyVec);
        // store
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dst + i), result);
    }

    // process remainder
    for (; i < size; i++) {
        dst[i] = src[i] ^ key;
    }
}

#elif defined(ASP_IS_ARM64)

#include <arm_neon.h>

void xor_u8_neon(uint8_t* src, uint8_t* dst, size_t size, uint8_t key) {
    uint8x16_t keyVec = vdupq_n_u8(key);

    size_t i = 0;
    size_t vecSize = sizeof(uint8x16_t);

    for (; i + vecSize <= size; i += vecSize) {
        // load data
        uint8x16_t data = vld1q_u8(src + i);
        // xor
        uint8x16_t result = veorq_u8(data, keyVec);
        // store
        vst1q_u8(dst + i, result);
    }

    // process remainder
    for (; i < size; i++) {
        dst[i] = src[i] ^ key;
    }
}

#endif

static void chooseImpl() {
#ifdef ASP_IS_X86
    if (blaze::simd().avx2) {
        xor_u8_impl = &xor_u8_avx2;
    } else {
        xor_u8_impl = &xor_u8_sse2;
    }
#elif defined (ASP_IS_ARM64)
    if (blaze::simd().neon) {
        xor_u8_impl = &xor_u8_neon;
    } else {
        xor_u8_impl = &xor_u8_scalar;
    }
#else
    xor_u8_impl = &xor_u8_scalar;
#endif
}

void blaze::xor_u8(uint8_t* src, uint8_t* dst, size_t size, uint8_t key) {
    if (!xor_u8_impl) [[unlikely]] chooseImpl();

    xor_u8_impl(src, dst, size, key);
}
