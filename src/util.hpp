#pragma once

#include <asp/time/Duration.hpp>
#include <asp/time/Instant.hpp>
#include <fmt/core.h>

#define BLAZE_HOOK_PRIO(func, p) \
    do { auto _xrs = self.setHookPriority(#func, p); \
    if (!_xrs) { \
        geode::log::error("hook apply failed: {}", _xrs.unwrapErr());\
    } \
    } while (0)

// Hook closer to original
#define BLAZE_HOOK_LAST(func) BLAZE_HOOK_PRIO(func, 999999)

// Hook closest to original
#define BLAZE_HOOK_VERY_LAST(func) BLAZE_HOOK_PRIO(func, 1999999999)

// Hook further from original
#define BLAZE_HOOK_FIRST(func) BLAZE_HOOK_PRIO(func, -999999)

// Hook furthest from original
#define BLAZE_HOOK_VERY_FIRST(func) BLAZE_HOOK_PRIO(func, -1999999999)

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

template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;

        return instance;
    }

protected:
    SingletonBase() {}
    virtual ~SingletonBase() {}
};

namespace blaze {
    [[noreturn]] void unreachable();

    // Returns size of the main executable image (Geometry Dash app)
    size_t imageSize();
    // Returns size of libcocos2d.dll (if Android, returns the same value as imageSize)
    size_t cocosImageSize();
}
