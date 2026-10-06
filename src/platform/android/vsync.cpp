#include <modules/load/LoadModule.hpp>
#include <EGL/egl.h>

using namespace geode::prelude;

namespace blaze {

void LoadModule::disableVsync() {
    eglSwapInterval(eglGetDisplay(EGL_DEFAULT_DISPLAY), 0);
}

void LoadModule::restoreVsync() {
    eglSwapInterval(eglGetDisplay(EGL_DEFAULT_DISPLAY), 1);
}

}
