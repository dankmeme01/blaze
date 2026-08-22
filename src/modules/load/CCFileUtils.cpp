#include <Geode/Geode.hpp>
#include <Geode/modify/CCFileUtils.hpp>
#include <AsyncLoad/FileUtils.hpp>
#include <util/macros.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

namespace blaze {

struct HookedCCFileUtils : Modify<HookedCCFileUtils, CCFileUtils> {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "cocos2d::CCFileUtils::fullPathForFilename",
            "cocos2d::CCFileUtils::getFileData"
        );

        (void) self.setHookPriority("cocos2d::CCFileUtils::fullPathForFilename", Priority::Replace);
    }

    $override
    gd::string fullPathForFilename(const char* filename, bool ignoreSuffix) {
        return AsyncLoad::fullPathForFilename(filename, ignoreSuffix);
    }

    $override
    unsigned char* getFileData(const char* pszFileName, const char* pszMode, unsigned long* pSize) {
        auto mode = std::string_view{pszMode};
        BLAZE_TRACE("getFileData(\"{}\", \"{}\")", pszFileName, pszMode);

        maybeChangeMode(pszFileName, mode);

        if (mode != "rb" && mode != "r" && mode != "rt") {
            log::warn("Unknown file mode: \"{}\" for file {}", pszMode, pszFileName);
            return CCFileUtils::getFileData(pszFileName, pszMode, pSize);
        }

        auto result = AsyncLoad::getFileDataOwned(pszFileName);
        if (!result) {
            log::warn("getFileData({}) failed: {}", pszFileName, result.unwrapErr());
            return nullptr;
        }

        auto buf = std::move(result).unwrap();

        if (mode != "rb") {
            buf = transformToText(buf);
        }

        if (pSize) *pSize = buf.size;

        return buf.data.release();
    }

    AsyncLoad::OwnedBuffer transformToText(AsyncLoad::OwnedBuffer& buf) {
        auto out = AsyncLoad::OwnedBuffer {
            std::make_unique<uint8_t[]>(buf.size + 1),
            0
        };

        auto push = [&](uint8_t c) {
            out.data[out.size++] = c;
        };

        for (size_t i = 0; i < buf.size; ++i) {
            auto c = buf.data[i];

            // skip \r if it directly precedes a \n
            if (c == '\r' && (i + 1 < buf.size) && buf.data[i + 1] == '\n') {
                continue;
            }

            out.data[out.size++] = c;
        }

        return out;
    }

    static void maybeChangeMode(const char* pszFileName, std::string_view& mode) {
        // for whatever reason, robtop decided to use "r" for binary save files, which is considerably slower and carries risk of corruption,
        // so we rewrite savefiles to use "rb" instead.

        std::string_view path{pszFileName};
        if (mode == "rb" || !path.ends_with(".dat")) return;

        auto slash = path.find_last_of("/\\");

        auto dir = slash == std::string_view::npos ? "" : path.substr(0, slash);
        auto wrpath = std::string{CCFileUtils::get()->getWritablePath()};
        while (wrpath.ends_with("/") || wrpath.ends_with("\\")) {
            wrpath.pop_back();
        }

        if (wrpath == dir) {
            BLAZE_TRACE("Changing file mode for '{}' to rb", pszFileName);
            mode = "rb";
        }
    }
};

}
