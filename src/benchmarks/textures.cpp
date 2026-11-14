#include "Benchmarks.hpp"
#include <asp/fs.hpp>
#include <asp/iter.hpp>
#include <AsyncLoad/Manager.hpp>
#include <AsyncLoad/FileUtils.hpp>

using namespace geode::prelude;

namespace blaze::bench {

void textures() {
    std::vector<std::string> names;
    for (size_t i = 1; i <= 19; i++) {
        names.push_back(fmt::format("PlayerExplosion_{:02}.png", i));
    }

    auto start = asp::Instant::now();

    AsyncLoad::TaskGroup group;
    for (auto& n : names) {
        auto path = AsyncLoad::fullPathForFilename(n);
        CCTextureCache::get()->removeTextureForKey(path.c_str());

        group.add(AsyncLoad::ALManager::get().loadTexture(path, [n](auto res) {}, true));
    }

    group.wait();
    log::debug("took {} for {} death effects", start.elapsed(), group.totalTasks());
}

}
