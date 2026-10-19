#pragma once
#include <string_view>
#include <fmt/format.h>

// assert macro

#define B_ASSERT(condition) \
    do { \
        if (!(condition)) [[unlikely]] { \
            ::blaze::_assertionFail(#condition, __FILE__, __LINE__); \
        } \
    } while (false)

#define B_ASSERT_EQ(actual, expect, msg) \
    do { \
        auto _a = (actual); \
        auto _b = (expect); \
        if (!(_a == _b)) [[unlikely]] { \
            ::blaze::_assertionFail(fmt::format(msg ": expected {}, got {}", _b, _a), __FILE__, __LINE__); \
        } \
    } while (false)

#ifdef BLAZE_DEBUG
# define B_DEBUG_ASSERT(c) B_ASSERT(c)
#else
# define B_DEBUG_ASSERT(c) ((void)0)
#endif

namespace blaze {
    [[noreturn]] void _assertionFail(std::string_view what, std::string_view file, int line);
}
