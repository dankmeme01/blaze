#include <modules/load/LoadModule.hpp>

using namespace geode::prelude;

namespace blaze {

static bool g_wasVsync = false;
static float g_wasInterval = 0.f;

// TODO: i couldnt properly bench this
void LoadModule::disableVsync() {
    // auto app = AppDelegate::get();
    // auto dir = CCDirector::get();
    // g_wasVsync = app->m_bVerticalSyncEnabled;
    // g_wasInterval = dir->getAnimationInterval();

    // log::debug("save vsync state: {} and fps limit: {}", g_wasVsync, 1.f / g_wasInterval);

    // app->toggleVerticalSync(false);
    // dir->setAnimationInterval(1.f / 1000.f); // 1000 fps
}

void LoadModule::restoreVsync() {
    // auto app = AppDelegate::get();
    // auto dir = CCDirector::get();

    // log::debug("restore vsync state: {} and fps limit: {}", g_wasVsync, 1.f / g_wasInterval);

    // app->toggleVerticalSync(g_wasVsync);
    // dir->setAnimationInterval(g_wasInterval);
}

}
