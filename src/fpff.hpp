#pragma once
#include <Geode/Geode.hpp>

// Faster rewrite of CCFileUtils::fullPathForFilename, optionally thread-safe
// Thread safety can be enabled and disabled globally, by calling `blaze::setFileUtilsThreadSafe(bool)`
// Or by constructing a `ThreadSafeFileUtilsGuard` object that will enable thread-safety for its lifetime

namespace blaze {

enum class TextureQuality {
    Low, Medium, High
};

void setFileUtilsThreadSafe(bool threadSafe);

struct ThreadSafeFileUtilsGuard {
    ThreadSafeFileUtilsGuard();
    ~ThreadSafeFileUtilsGuard();

private:
    bool m_previousState;
};

gd::string fullPathForFilename(std::string_view input, bool ignoreSuffix = false);
gd::string getPathForFilename(std::string_view filename, std::string_view resolutionDir, std::string_view searchPath);

}