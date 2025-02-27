#pragma once

#include <Geode/platform/cplatform.h>

#ifdef GEODE_IS_WINDOWS
# define BLAZE_ASYNC_GLFW_SUPPORTED 1

extern bool g_canHookGlfw;

namespace blaze {
    void customGlfwInit();
}

#endif // GEODE_IS_WINDOWS
