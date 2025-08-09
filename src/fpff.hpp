#pragma once
#include <Geode/Geode.hpp>

// Thread safe, fast rewrite of CCFileUtils::fullPathForFilename

namespace blaze {

enum class TextureQuality {
    Low, Medium, High
};

gd::string fullPathForFilename(std::string_view input, bool ignoreSuffix = false);

}