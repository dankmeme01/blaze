#include <Geode/Geode.hpp>

#include <AsyncLoad/FileUtils.hpp>
#include <asp/sync/Mutex.hpp>
#include <asp/fs/DirectoryIterator.hpp>
#include <gtl/phmap.hpp>
#include <util/macros.hpp>

using namespace geode::prelude;
using namespace asp::fs;
using namespace AsyncLoad;

struct DirSnapshot {
    gtl::flat_hash_set<Path> files;

    bool contains(PathView file) const {
        return this->files.contains(file);
    }

    void add(Path file) {
        this->files.emplace(std::move(file));
    }
};

static asp::Mutex<gtl::flat_hash_map<Path, DirSnapshot>> g_dirs;

static std::pair<PathView, PathView> splitPath(std::string_view path) {
    auto slash = path.find_last_of("/\\");
    if (slash == std::string_view::npos) {
        return { "", path };
    }
    return { path.substr(0, slash), path.substr(slash + 1) };
}

static Path normalizeFile(PathView file) {
    std::string out{file};
    utils::string::toLowerIP(out);
    return out;
}

static Path normalizeDir(PathView dir) {
    std::string str{dir};
    utils::string::toLowerIP(str);

    Path out{str};
    out.makePreferred();

    return out;
}

static geode::Result<DirSnapshot> makeSnapshotFor(PathView dir) {
    BLAZE_IF_DEBUG(auto start = asp::Instant::now());

    DirSnapshot snap;

    auto res = asp::fs::DirectoryIterator::create(dir, {
        .m_largeBuffer = true,
    });
    if (!res && res.unwrapErr() == asp::fs::ErrorKind::NotFound) {
        return Ok(std::move(snap));
    }

    auto iterator = GEODE_UNWRAP(std::move(res));
    for (auto r : iterator) {
        if (!r) {
            log::warn("error on entry inside '{}': {}", dir, r.unwrapErr());
            continue;
        }

        auto& entry = r.unwrap();

        // filter out directories
        if (entry.isDir()) continue;

        // strip away the base
        auto filename  = entry.path.filename().value();
        snap.add(Path{filename});
    }

    BLAZE_TRACE("Snapshotted directory {} in {} ({} files)", dir, start.elapsed(), snap.files.size());
    return Ok(std::move(snap));
}

bool exists(ZStringView path) {
#ifdef GEODE_IS_ANDROID
    // this optimization is a bit pointless for AAsset paths, so don't do dir snapshots if it's an apk asset
    if (!path.view().starts_with('/')) {
        return AsyncLoad::impl::fileExists(path);
    }
#endif

    auto [dirBare, file] = splitPath(path);

    auto dir = normalizeDir(dirBare);

    auto dirs = g_dirs.lock();
    auto it = dirs->find(dir);
    if (it != dirs->end()) {
        return it->second.contains(file);
    }

    // make a snapshot
    auto snapshot = makeSnapshotFor(dir);
    if (!snapshot) {
        auto code = snapshot.unwrapErr();
        log::warn("Failed to make snapshot for '{}': error {}", dir, code);
        return false;
    }

    auto [it2, _] = dirs->emplace(dir, std::move(snapshot.unwrap()));
    return it2->second.contains(file);
}

void purgeCache() {
    g_dirs.lock()->clear();
}

$on_mod(Loaded) {
    if (Mod::get()->getSettingValue<bool>("directory-snapshotting")) {
        FileUtilsProvider provider{};
        provider.exists = &exists;
        provider.clearCache = &purgeCache;
        AsyncLoad::setFileUtilsProvider(&provider);

        // pre-fill a few asynchronously
        arc::spawnBlocking<void>([] {
            auto resources = dirs::getResourcesDir().string();
            if (resources.back() == '\\' || resources.back() == '/') {
                resources.pop_back();
            }

            exists(fmt::format("{}/dummy", resources));
            exists(fmt::format("{}/icons/dummy", resources));
            exists(fmt::format("{}/sfx/dummy", resources));
            exists(fmt::format("{}/levels/dummy", resources));
            exists(fmt::format("{}/mapimages/dummy", resources));
            exists(fmt::format("{}/songs/dummy", resources));
        });
    }
}
