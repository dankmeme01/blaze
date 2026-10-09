#include "Benchmarks.hpp"

using namespace geode::prelude;

namespace blaze::bench {

void fpff() {
    auto fu = CCFileUtils::get();

    std::vector<std::string> testNames;
    std::filesystem::directory_iterator it{dirs::getResourcesDir() / "icons"};
    for (auto& ent : it) {
        auto fileName = ent.path().filename().string();
        if (fileName.contains("-hd") || fileName.contains("-uhd")) continue;
        auto withoutDir = ent.path().native().substr(dirs::getResourcesDir().native().size() + 1);
        testNames.push_back(utils::string::wideToUtf8(withoutDir));
    }

    auto t1 = asp::Instant::now();
    for (int i = 0; i < 1024; i++) {
        fu->fullPathForFilename("icons/fake/ass/png", false);
    }
    auto t2 = asp::Instant::now();
    for (int i = 0; i < 1024; i++) {
        fu->fullPathForFilename("GJ_GameSheet03-uhd.png", false);
    }
    auto t3 = asp::Instant::now();
    for (auto& t : testNames) {
        fu->fullPathForFilename(t.c_str(), false);
    }
    auto t4 = asp::Instant::now();

    log::info("Time for non-existing paths: {}", t2.durationSince(t1) / 1024);
    log::info("Time for repeating same path: {}", t3.durationSince(t2) / 1024);
    log::info("Time for {} test paths: {}", testNames.size(), t4.durationSince(t3) / testNames.size());
}

}
