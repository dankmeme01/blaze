#include "fpff.hpp"

using namespace geode::prelude;

namespace blaze {

struct HookedFileUtils : public CCFileUtils {
    static HookedFileUtils& get() {
        return *static_cast<HookedFileUtils*>(CCFileUtils::get());
    }

    bool fileExists(const char* path) {
#ifdef GEODE_IS_WINDOWS
        auto attrs = GetFileAttributesA(path);

        return (attrs != INVALID_FILE_ATTRIBUTES &&
                !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat st;
        return (stat(path, &st) == 0) && S_ISREG(st.st_mode);
#endif
    }

    $override
    gd::string getPathForFilename(const gd::string& filename, const gd::string& resolutionDirectory, const gd::string& searchPath) {
        std::string_view file = filename;
        std::string_view filePath;

        size_t slashPos = file.find_last_of('/');
        if (slashPos != std::string::npos) {
            filePath = file.substr(0, slashPos + 1);
            file = file.substr(slashPos + 1);
        }

        std::array<char, 1024> buf;
        auto result = fmt::format_to_n(buf.data(), buf.size(), "{}{}{}{}", searchPath, filePath, resolutionDirectory, file);
        if (result.size < buf.size()) {
            buf[result.size] = '\0';
        } else {
            buf[buf.size() - 1] = '\0';
        }

        if (this->fileExists(buf.data())) {
            return gd::string(buf.data(), result.size);
        } else {
            return gd::string{};
        }
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

// transforms a string like "icon-41" into "icon-41-hd.png" depending on the current texture quality.
static void appendQualitySuffix(std::string& out, TextureQuality quality, bool plist) {
    switch (quality) {
        case TextureQuality::Low: {
            if (plist) out.append(".plist");
            else out.append(".png");
        } break;
        case TextureQuality::Medium: {
            if (plist) out.append("-hd.plist");
            else out.append("-hd.png");
        } break;
        case TextureQuality::High: {
            if (plist) out.append("-uhd.plist");
            else out.append("-uhd.png");
        } break;
    }
}

gd::string fullPathForFilename(std::string_view input, bool ignoreSuffix) {
    auto& fu = HookedFileUtils::get();

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

    // add the quality suffix if needed
    std::string filename;

    if (!ignoreSuffix) {
        bool hasQualitySuffix = input.ends_with("-hd.png") ||
                                input.ends_with("-uhd.png") ||
                                input.ends_with("-hd.plist") ||
                                input.ends_with("-uhd.plist");

        if (!hasQualitySuffix) {
            if (input.ends_with(".plist")) {
                filename = input.substr(0, input.find(".plist"));
                appendQualitySuffix(filename, getTextureQuality(), true);
            } else {
                filename = input.substr(0, input.find(".png"));
                appendQualitySuffix(filename, getTextureQuality(), false);
            }
        }
    }

    if (filename.empty()) {
        filename = input;
    }

    // TODO: we disregard CCFileUtils m_pFilenameLookupDict / getNewFilename() here

    std::string fullpath;
    auto& searchPaths = fu.getSearchPaths();

    // we discard resolution directories here, since no one uses them
#define TRY_PATH(sp) \
    auto _fp = fu.getPathForFilename(filename, "", sp); \
    if (!_fp.empty()) { \
        return _fp; \
    }

    // try all search paths
    for (const auto& sp : searchPaths) {
        TRY_PATH(sp);
    }

    if (ignoreSuffix) {
        // if all else fails, accept defeat
        log::warn("blaze: could not find full path for '{}' (transformed: '{}')", input, filename);
        return filename;
    } else {
        // try to find the file without the quality suffix
        return fullPathForFilename(input, true);
    }
}

}
