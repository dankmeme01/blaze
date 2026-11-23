#include <Geode/Geode.hpp>
#include <Geode/modify/GManager.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(GManager) {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(self);

        (void) self.setHookPriority("GManager::loadDataFromFile", Priority::Replace);
    }

    void loadDataFromFile(const std::string& filename) {
        auto start = asp::Instant::now();

        auto dict = LoadModule::get().loadDSDictionarySync(filename);
        if (dict) {
            log::info("Loaded {} in {}", filename, start.elapsed());
            this->dataLoaded(dict.get());
        } else {
            log::warn("Failed to load {} after {}, assuming first load", filename, start.elapsed());
            this->firstLoad();
        }
    }
};

}
