#include "AudioModule.hpp"
#include <Geode/modify/FMODAudioEngine.hpp>

using namespace geode::prelude;

namespace blaze {

AudioModule::AudioModule() {}

class $modify(FMODAudioEngine) {
    static void onModify(auto& self) {
        AudioModule::get().addHooks(self);
    }
};

}

$on_mod(Loaded) {
    // Mod::get()->hook(addresser::getNonVirtual(&FMOD::System))
}