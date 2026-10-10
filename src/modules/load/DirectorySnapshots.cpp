#include <Geode/Geode.hpp>

#ifdef GEODE_IS_WINDOWS

#include <AsyncLoad/FileUtils.hpp>
#include <asp/sync/Mutex.hpp>
#include <gtl/phmap.hpp>
#include <util/macros.hpp>

using namespace geode::prelude;
using namespace AsyncLoad;

struct DirSnapshot {
    gtl::flat_hash_set<std::string> files;

    bool contains(std::string_view file) const {
        return this->files.contains(file);
    }

    void add(std::string file) {
        this->files.emplace(std::move(file));
    }
};

static asp::Mutex<gtl::flat_hash_map<std::string, DirSnapshot>> g_dirs;

static std::pair<std::string_view, std::string_view> splitPath(std::string_view path) {
    auto slash = path.find_last_of("/\\");
    if (slash == std::string_view::npos) {
        return { "", path };
    }
    return { path.substr(0, slash), path.substr(slash + 1) };
}

static std::string normalizeFile(std::string_view file) {
    std::string out{file};
    utils::string::toLowerIP(out);
    return out;
}

static std::string normalizeDir(std::string_view dir) {
    std::string out{dir};
    utils::string::toLowerIP(out);

    for (auto& c : out) {
        if (c == '/') c = '\\';
    }

    return out;
}

static Result<DirSnapshot> makeSnapshotFor(std::string_view dir) {
    BLAZE_IF_DEBUG(auto start = asp::Instant::now());

    // why yes
    utils::StringBuffer<> buf;
    buf.append(dir);
    buf.append("\\*");

    auto wideRes = utils::string::utf8ToUtf16(buf.view());
    if (!wideRes) {
        return Err("utf-8 -> utf-16 conversion failed for '{}': {}", buf.view(), wideRes.unwrapErr());
    }

    auto wide = std::move(wideRes).unwrap();

    WIN32_FIND_DATAW data{};
    auto hfind = FindFirstFileExW(
        (wchar_t*)wide.c_str(),
        FindExInfoBasic,
        &data,
        FindExSearchNameMatch,
        nullptr,
        FIND_FIRST_EX_LARGE_FETCH
    );

    if (hfind == INVALID_HANDLE_VALUE) {
        auto code = GetLastError();
        // non-existing directory counts as empty directory for our intents and purposes
        if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) {
            BLAZE_TRACE_NOISY("Snapshotted non-existing directory {} in {}", dir, start.elapsed());
            return Ok(DirSnapshot{});
        }

        return Err("FindFirstFileExW failed: error {}", code);
    }

    DirSnapshot snap;
    do {
        std::wstring_view filename { data.cFileName };
        if (filename == L"." || filename == L"..") continue;

        // filter out directories
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::u16string_view utf16View{(const char16_t*)filename.data(), filename.size()};
        auto res = utils::string::utf16ToUtf8(utf16View);
        if (!res) {
            return Err("utf-16 -> utf-8 conversion failed for a file inside '{}': {}", dir, res.unwrapErr());
        }

        snap.add(std::move(res).unwrap());
    } while (FindNextFileW(hfind, &data));

    FindClose(hfind);

    BLAZE_TRACE("Snapshotted directory {} in {} ({} files)", dir, start.elapsed(), snap.files.size());
    return Ok(std::move(snap));
}

bool exists(ZStringView path) {
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
            auto resources = utils::string::pathToString(dirs::getResourcesDir());
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

#endif
