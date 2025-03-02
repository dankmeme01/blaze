#pragma once

#include <Geode/loader/Log.hpp>
#include <cstdlib>

#define BLAZE_ASSERT_ALWAYS(cond) \
    do { \
        if (!(cond)) { \
            blaze::assertionFailed(__FILE__, __LINE__, #cond); \
        } \
    } while (0)

namespace blaze {
    [[noreturn]] inline void assertionFailed(const char* file, int line, const char* cond) {
        geode::log::error("Assertion failed at {}:{}: {}", file, line, cond);

#ifdef GEODE_IS_WINDOWS
        // Exceptions work pretty well on Windows, we can throw one. On other platforms we just abort.
        throw std::runtime_error(fmt::format("Assertion failed at {}:{}: {}", file, line, cond));
#else
        std::abort();
#endif
    }
}

#ifdef BLAZE_DEBUG
# define BLAZE_ASSERT(cond) BLAZE_ASSERT_ALWAYS(cond)
#else
# define BLAZE_ASSERT(cond) do {} while (0)
#endif
