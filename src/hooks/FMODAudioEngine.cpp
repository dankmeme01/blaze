#include <Geode/Geode.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>

#include <TaskTimer.hpp>

class $modify(FMODAudioEngine) {
    void setup() {
        BLAZE_TIMER_START("setup");
        FMODAudioEngine::setup();
    }
    void setupAudioEngine() {
        BLAZE_TIMER_START("setupAudioEngine");
        FMODAudioEngine::setupAudioEngine();
    }
};
