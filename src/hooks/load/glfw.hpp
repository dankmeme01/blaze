#pragma once

#include <Geode/platform/cplatform.h>

#ifdef GEODE_IS_WINDOWS
# define BLAZE_ASYNC_GLFW_SUPPORTED 1

extern bool g_canHookGlfw;
#include <Geode/cocos/robtop/glfw/glfw3.h>

namespace blaze {
    void customGlfwInit();
}

#endif // GEODE_IS_WINDOWS
