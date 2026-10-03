#pragma once
#include <string_view>

// assert macro

#define B_ASSERT(condition) \
    do { \
        if (!(condition)) [[unlikely]] { \
            ::blaze::_assertionFail(#condition, __FILE__, __LINE__); \
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
