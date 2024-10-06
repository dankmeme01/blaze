#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>

#include <mutex>
#include <util.hpp>

class $modify(HookedFMODAudioEngine, FMODAudioEngine) {
    struct Fields {
        std::mutex initMutex;
        std::atomic_bool initialized = false;
    };

    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(FMODAudioEngine::setupAudioEngine);
    }

    static HookedFMODAudioEngine* get() {
        return static_cast<HookedFMODAudioEngine*>(FMODAudioEngine::get());
    }

    // Hooked and reimplemented to be asynchronous.
    $override
    void setupAudioEngine();

    void setupAudioEngineReimpl(bool gv0159, bool reducedQuality, bool inThread);
};
