#include <Geode/Geode.hpp>
#include <Geode/modify/CCFileUtils.hpp>
#include <AsyncLoad/FileUtils.hpp>
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
        if (std::string_view{pszMode} != "rb") {
            return CCFileUtils::getFileData(pszFileName, pszMode, pSize);
        }

        auto result = AsyncLoad::getFileDataOwned(pszFileName);
        if (!result) {
            log::warn("getFileData({}) failed: {}", pszFileName, result.unwrapErr());
            return nullptr;
        }

        auto buf = std::move(result).unwrap();
        if (pSize) *pSize = buf.size;

        return buf.data.release();
    }
};

}
