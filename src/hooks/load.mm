#include "load.hpp"
#include <Geode/Geode.hpp>

#ifdef GEODE_IS_MACOS
# import <Foundation/Foundation.h>
#endif

static void(*s_applicationDidFinishLaunchingOrig)(void*, SEL, NSNotification*);

void appControllerHook(void* self, SEL sel, NSNotification* notif) {
    blaze::startPreInit();

    // finally go back to running the rest of the game
    return s_applicationDidFinishLaunchingOrig(self, sel, notif);
}

$execute {
    s_applicationDidFinishLaunchingOrig = reinterpret_cast<decltype(s_applicationDidFinishLaunchingOrig)>(geode::base::get() + 0x98cc);

    (void) Mod::get()->hook(
        reinterpret_cast<void*>(s_applicationDidFinishLaunchingOrig),
        &appControllerHook,
        "AppController::applicationDidFinishLaunching",
        tulip::hook::TulipConvention::Default
    );
}
