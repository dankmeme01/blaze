#pragma once

#include <asp/simd.hpp>

#if defined(__clang__) || defined(__GNUC__)
# define BLAZE_SSE2 __attribute__((__target__("sse2")))
# define BLAZE_SSSE3 __attribute__((__target__("ssse3")))
# define BLAZE_SSE41 __attribute__((__target__("sse4.1")))
# define BLAZE_SSE42 __attribute__((__target__("sse4.2")))
# define BLAZE_AVX __attribute__((__target__("avx")))
# define BLAZE_AVX2 __attribute__((__target__("avx2")))
# define BLAZE_AVX512F __attribute__((__target__("avx512f")))
#else // __clang__
// on msvc there's no need to set these
# define BLAZE_SSE2
# define BLAZE_SSSE3
# define BLAZE_SSE41
# define BLAZE_SSE42
# define BLAZE_AVX
# define BLAZE_AVX2
# define BLAZE_AVX512F
#endif // __clang__

namespace blaze {

struct Simd {
#ifdef ASP_IS_X86
    bool sse3;
    bool sse4_1;
    bool sse4_2;
    bool ssse3;
    bool avx;
    bool avx2;
    bool avx512;
    bool avx512dq;

    Simd() {
        auto f = asp::simd::getFeatures();
        sse3 = f.sse3;
        sse4_1 = f.sse4_1;
        sse4_2 = f.sse4_2;
        ssse3 = f.ssse3;
        avx = f.avx;
        avx2 = f.avx2;
        avx512 = f.avx512;
        avx512dq = f.avx512dq;
    }
#elif defined ASP_IS_ARM
    bool neon = false;

    Simd() {
        GEODE_ANDROID64(neon = true);
    }
#endif
};

inline Simd& simd() {
    static Simd instance;
    return instance;
}

}