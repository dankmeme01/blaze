#pragma once
#include <Geode/utils/general.hpp>
#include <cocos2d.h>

namespace blaze {

static constexpr inline cocos2d::ccHSVValue DEFAULT_HSV {
    .h = 0,
    .s = 1,
    .v = 1,
    .absoluteSaturation = false,
    .absoluteBrightness = false,
};

inline cocos2d::ccHSVValue parseHsv(std::string_view sv, char delim = 'a') {
    if (sv.empty()) return DEFAULT_HSV;

    auto chunk = asp::iter::split(sv, delim).arrayChunks<5>().next();
    if (!chunk) return DEFAULT_HSV;

    auto [h, s, v, absSat, absBri] = *chunk;

    return {
        .h = geode::utils::numFromString<float>(h).unwrapOr(0.0f),
        .s = geode::utils::numFromString<float>(s).unwrapOr(0.0f),
        .v = geode::utils::numFromString<float>(v).unwrapOr(0.0f),
        .absoluteSaturation = geode::utils::numFromString<int>(absSat).unwrapOr(0) != 0,
        .absoluteBrightness = geode::utils::numFromString<int>(absBri).unwrapOr(0) != 0,
    };
}

}
