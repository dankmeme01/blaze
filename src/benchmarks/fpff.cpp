#include "Benchmarks.hpp"
#include <asp/fs.hpp>
#include <asp/iter.hpp>

using namespace geode::prelude;

namespace blaze::bench {

void fpff() {
    auto fu = CCFileUtils::get();

    std::vector<std::string> testNames = asp::fs::iterDir(dirs::getResourcesDir() / "icons").unwrap()
        .filterMap([](auto entry) { return entry.ok(); })
        .filterMap([](auto entry) -> std::optional<asp::fs::Path> {
            auto fileName = entry.path.filename().value().string();
            if (fileName.contains("-hd") || fileName.contains("-uhd")) return std::nullopt;

            return entry.path;
        })
        .map([](auto path) {
            return path.string().substr(dirs::getResourcesDir().string().size() + 1);
        })
        .collect();

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
