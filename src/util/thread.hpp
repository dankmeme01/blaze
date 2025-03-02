#pragma once

#include "assert.hpp"
#include <thread>

#ifdef BLAZE_DEBUG
# define BLAZE_ASSERT_MAIN_THREAD BLAZE_ASSERT(blaze::isMainThread())
#else
# define BLAZE_ASSERT_MAIN_THREAD do {} while (0)
#endif

namespace blaze {
    extern std::thread::id mainThreadId;

    void setMainThreadId();
    bool isMainThread();
}
