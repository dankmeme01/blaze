#include "load.hpp"
#include <Geode/Geode.hpp>

#ifdef GEODE_IS_MACOS
#define CommentType CommentTypeDummy
# import <Foundation/Foundation.h>
#undef CommentType

static void(*s_applicationDidFinishLaunchingOrig)(void*, SEL, NSNotification*);

void appControllerHook(void* self, SEL sel, NSNotification* notif) {
    blaze::startPreInit();

    // finally go back to running the rest of the game
    return s_applicationDidFinishLaunchingOrig(self, sel, notif);
}

$execute {
#if GEODE_GD_COMP_VERSION != 22074
# error "This hook is not compatible with this version of GD"
#endif

    s_applicationDidFinishLaunchingOrig = reinterpret_cast<decltype(s_applicationDidFinishLaunchingOrig)>(geode::base::get() +
#ifdef GEODE_IS_ARM_MAC
        0x98cc
#else
        0x7af0
#endif
);

    (void) geode::Mod::get()->hook(
        reinterpret_cast<void*>(s_applicationDidFinishLaunchingOrig),
        &appControllerHook,
        "AppController::applicationDidFinishLaunching",
        tulip::hook::TulipConvention::Default
    );
}

#endif
