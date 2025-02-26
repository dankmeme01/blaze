#include "glfw.hpp"

#ifdef BLAZE_ASYNC_GLFW_SUPPORTED

#include <util.hpp>
#include <sinaps.hpp>
#include <tracing.hpp>
#include <TaskTimer.hpp>
#include <settings.hpp>

using namespace geode::prelude;

bool g_canHookGlfw = false;

# ifdef GEODE_IS_WINDOWS
auto _glfwInitWGL_O = static_cast<bool (*)()>(nullptr);
auto glfwInit_O = static_cast<int (*)()>(nullptr);
auto _glfwCreateWindowWin32_O = static_cast<bool (*)(GLFWwindow*, const void*, const void*, const void*)>(nullptr);
auto _glfwCreateContextWGL_O = static_cast<bool (*)(GLFWwindow*, const void*, const void*)>(nullptr);
auto createNativeWindow_O = static_cast<bool (*)(GLFWwindow*, const void*, const void*)>(nullptr);
auto _glfwRefreshContextAttribs_O = static_cast<bool (*)(GLFWwindow*, const void*)>(nullptr);

bool _glfwCreateWindowWin32(GLFWwindow* window,
    const void* wndconfig,
    const void* ctxconfig,
    const void* fbconfig)
{
    ZoneScoped;
    BLAZE_TIMER_START("glfwCreateWindowWin32");

    auto wnd = createNativeWindow_O(window, wndconfig, fbconfig);
    if (!wnd) return false;

    int int1 = reinterpret_cast<const int*>(ctxconfig)[0];
    int int2 = reinterpret_cast<const int*>(ctxconfig)[1];

    if (int1 != 0) {
        if (int2 != 0x36001) {
            log::error("Type is wrong: {:X}", int2);
            std::abort(); // this should never be achievable on windows
        }

        BLAZE_TIMER_STEP("glfwInitWGL");
        if (!_glfwInitWGL_O()) {
            return false;
        }

        BLAZE_TIMER_STEP("glfwCreateContextWGL");
        if (!_glfwCreateContextWGL_O(window, ctxconfig, fbconfig)) {
            return false;
        }

        BLAZE_TIMER_STEP("glfwRefreshContextAttribs");
        if (!_glfwRefreshContextAttribs_O(window, ctxconfig)) {
            return false;
        }
    }

    BLAZE_TIMER_STEP("Misc tasks");
    auto hwnd = *(HWND*)((char*)window + 0x370);
    ShowWindow(hwnd, 0x8);
    BringWindowToTop(hwnd);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);

    return true;
}

// For speed, we won't scan the entire binary. This is a little bit volatile, but we assume that all the functions appear after 0xd0000
// (Offsets that were found in 2.2074: 0xda880, 0xd1390, 0xd3f80, 0xda400, 0xd5360, 0xd0680)
constexpr static size_t SEARCH_POS_START = 0xd0000;

// Bind the functions with runtime sigscanning.

template <typename T>
void _bindFunction(T& func, const char* name, uintptr_t base, intptr_t address) {
    if (address == -1) {
        func = nullptr;
        g_canHookGlfw = false;
        log::warn("Sigscanning failed to find {}, GLFW hooking is unavailable.", name);
    } else {
        func = reinterpret_cast<T>(base + address);
    }
}

$execute {
    g_canHookGlfw = true;

    auto base = geode::base::getCocos();
    auto size = blaze::cocosImageSize();

    base += SEARCH_POS_START;
    size -= SEARCH_POS_START;

    auto baseptr = (uint8_t*)base;

    _bindFunction(_glfwInitWGL_O, "_glfwInitWGL", base,
        sinaps::find<
            "40 57 48 83 EC 50 48 8B ? ? ? ? ? 48 33 C4"
            >(baseptr, size)
    );

    _bindFunction(glfwInit_O, "glfwInit", base,
        sinaps::find<
            "48 83 EC 28 83 3D ? ? ? ? ? 0F 85"
            >(baseptr, size)
    );

    _bindFunction(_glfwCreateWindowWin32_O, "_glfwCreateWindowWin32", base,
        sinaps::find<
            "48 89 ? ? ? 48 89 ? ? ? 48 89 ? ? ? 57 48 83 EC 20 49 8B F8 49 8B E9"
            >(baseptr, size)
    );

    _bindFunction(_glfwCreateContextWGL_O, "_glfwCreateContextWGL", base,
        sinaps::find<
            "48 89 ? ? ? 55 56 57 48 81 EC 00 01 00 00 48 8B ? ? ? ? ? 48 33 C4 48 89 ? ? ? ? ? ? 48 8B ? ? 33 ED"
            >(baseptr, size)
    );

    _bindFunction(createNativeWindow_O, "createNativeWindow", base,
        sinaps::find<
            "40 55 53 56 57 41 55 41 56"
            >(baseptr, size)
    );

    _bindFunction(_glfwRefreshContextAttribs_O, "_glfwRefreshContextAttribs", base,
        sinaps::find<
            "40 53 55 57 41 55 41 56 48 83"
            >(baseptr, size)
    );

    if (blaze::settings().asyncGlfw && g_canHookGlfw) {
        (void) Mod::get()->hook(
            reinterpret_cast<void*>(_glfwCreateWindowWin32_O),
            _glfwCreateWindowWin32,
            "_glfwCreateWindowWin32"
        );
    }
}

# else
#  error "Async GLFW not implemented on this platform despite being marked as supported!"
# endif // GEODE_IS_WINDOWS

void blaze::customGlfwInit() {
    int result = glfwInit_O();

    if (result == 1) {
        bool result2 = _glfwInitWGL_O();
        if (!result2) {
            log::warn("_glfwInitWGL failed.");
        }
    } else {
        log::warn("glfwInit failed: {}", result);
    }
}

#endif // BLAZE_ASYNC_GLFW_SUPPORTED
