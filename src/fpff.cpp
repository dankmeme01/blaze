#include <Geode/modify/CCFileUtils.hpp>
#include <asp/sync/SpinLock.hpp>
#include <asp/time.hpp>
#include "fpff.hpp"

using namespace geode::prelude;

static std::unordered_map<uint64_t, gd::string> g_cache;
static asp::SpinLock<> g_lock;
static std::atomic<bool> g_threadSafe{false};

namespace blaze {

ThreadSafeFileUtilsGuard::ThreadSafeFileUtilsGuard() {
    m_previousState = g_threadSafe.exchange(true, std::memory_order::acq_rel);
}

ThreadSafeFileUtilsGuard::~ThreadSafeFileUtilsGuard() {
    g_threadSafe.store(m_previousState, std::memory_order::release);
}

struct HookedFileUtils : public Modify<HookedFileUtils, CCFileUtils> {
    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::get());
    }

    $override
    gd::string getPathForFilename(const gd::string& filename, const gd::string& resolutionDirectory, const gd::string& searchPath) {
        return blaze::getPathForFilename(filename, resolutionDirectory, searchPath);
    }

    $override
    gd::string fullPathForFilename(const char* pszFileName, bool skipSuffix) {
        return blaze::fullPathForFilename(pszFileName, skipSuffix);
    }

    $override
    void purgeCachedEntries() {
        CCFileUtils::purgeCachedEntries();
        auto guard = g_lock.lock();
        g_cache.clear();
    }
};

static TextureQuality getTextureQuality() {
    float sf = CCDirector::get()->getContentScaleFactor();
    if (sf >= 4.f) {
        return TextureQuality::High;
    } else if (sf >= 2.f) {
        return TextureQuality::Medium;
    } else {
        return TextureQuality::Low;
    }
}

static uint64_t fnv1aHash(std::string_view s) {
    uint64_t hash = 0xcbf29ce484222325;
    for (char c : s) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3;
    }
    return hash;
}

template <size_t N>
static void appendToBuf(std::array<char, N>& buf, size_t& offset, std::string_view str) {
    size_t toCopy = std::min(str.size(), N - offset - 1);
    std::memcpy(buf.data() + offset, str.data(), toCopy);
    offset += toCopy;
    buf[offset] = '\0';
}

// returns the quality suffix for the given quality, e.g. "", "-hd", "-uhd"
static std::string_view getQualitySuffix(TextureQuality quality) {
    switch (quality) {
        case TextureQuality::Low: {
            return "";
        } break;
        case TextureQuality::Medium: {
            return "-hd";
        } break;
        case TextureQuality::High: {
            return "-uhd";
        } break;
    }
}

void setFileUtilsThreadSafe(bool threadSafe) {
    g_threadSafe.store(threadSafe, std::memory_order::release);
}

static void cachePath(uint64_t hash, const gd::string& path) {
    if (g_threadSafe.load(std::memory_order::acquire)) {
        auto guard = g_lock.lock();
        g_cache.emplace(hash, path);
    } else {
        g_cache.emplace(hash, path);
    }
}

static gd::string getCachedPath(uint64_t hash) {
    if (g_threadSafe.load(std::memory_order::acquire)) {
        auto guard = g_lock.lock();
        auto it = g_cache.find(hash);
        if (it != g_cache.end()) {
            return it->second;
        }
    } else {
        auto it = g_cache.find(hash);
        if (it != g_cache.end()) {
            return it->second;
        }
    }

    return {};
}

gd::string fullPathForFilename(std::string_view input, bool ignoreSuffix) {
    if (input.empty()) {
        return {};
    }

    // if the input is an absolute path, return it as is
    // we try to make this check as cheap as possible, so don't rely on std::filesystem or cocos
#ifdef GEODE_IS_WINDOWS
    if (input.size() >= 3 && std::isalpha(input[0]) && input[1] == ':' && (input[2] == '/' || input[2] == '\\')) {
        return gd::string{input};
    } else if (input.size() >= 2 && input[0] == '\\' && input[1] == '\\') {
        return gd::string{input};
    }
#else
    if (input.size() >= 1 && input[0] == '/') {
        return gd::string{input.data(), input.size()};
    }
#endif

    // try to find the string in cache
    auto hash = fnv1aHash(input);
    if (ignoreSuffix) {
        hash ^= 0xdeadbeefdeadbeef;
    }

    auto cached = getCachedPath(hash);
    if (!cached.empty()) {
        return cached;
    }

    auto& fu = HookedFileUtils::get();

    // add the quality suffix if needed
    std::array<char, 1024> filenameBuf;
    filenameBuf[0] = '\0';
    size_t filenameOffset = 0;

    auto append = [&](std::string_view str) {
        appendToBuf(filenameBuf, filenameOffset, str);
    };

    if (!ignoreSuffix) {
        std::string_view extension = input.substr(input.find_last_of('.'));
        std::string_view base = input.substr(0, input.find_last_of('.'));

        bool hasQualitySuffix = base.ends_with("-uhd") || base.ends_with("-hd");

        if (!hasQualitySuffix) {
            append(base);
            append(getQualitySuffix(getTextureQuality()));
            append(extension);
        }
    }

    if (filenameOffset == 0) {
        append(input);
    }

    // we disregard CCFileUtils m_pFilenameLookupDict / getNewFilename() here,
    // as nobody really uses it and it'd be a pain

    std::string_view filename{filenameBuf.data(), filenameOffset};
    auto& searchPaths = fu.getSearchPaths();

    // we discard resolution directories here, since no one uses them

    // try all search paths
    for (const auto& sp : searchPaths) {
        auto fp = blaze::getPathForFilename(filename, "", sp);
        if (!fp.empty()) {
            cachePath(hash, fp);
            return fp;
        }
    }

    if (ignoreSuffix) {
        // if all else fails, accept defeat
        cachePath(hash, gd::string{filename.data(), filename.size()});
        return gd::string{filename.data(), filename.size()};
    } else {
        // try to find the file without the quality suffix
        auto ret = fullPathForFilename(input, true);
        cachePath(hash, ret);
        return ret;
    }
}


static bool fileExists(const char* path) {
#ifdef GEODE_IS_WINDOWS
    auto attrs = GetFileAttributesA(path);

    return (attrs != INVALID_FILE_ATTRIBUTES &&
            !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#endif
}

gd::string getPathForFilename(std::string_view file, std::string_view resolutionDir, std::string_view searchPath) {
    std::string_view filePath;

    size_t slashPos = file.find_last_of('/');
    if (slashPos != std::string::npos) {
        filePath = file.substr(0, slashPos + 1);
        file = file.substr(slashPos + 1);
    }

    std::array<char, 1024> buf;
    auto result = fmt::format_to_n(buf.data(), buf.size(), "{}{}{}{}", searchPath, filePath, resolutionDir, file);
    if (result.size < buf.size()) {
        buf[result.size] = '\0';
    } else {
        buf[buf.size() - 1] = '\0';
    }

    if (fileExists(buf.data())) {
        return gd::string(buf.data(), result.size);
    } else {
        return gd::string{};
    }
}

}
