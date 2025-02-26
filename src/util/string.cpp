#include "string.hpp"
#include <fast_float/fast_float.h>

using namespace geode::prelude;

namespace blaze {

std::optional<float> parseFloat(std::string_view str) {
    float out = 0.0f;
    auto res = fast_float::from_chars(str.data(), str.data() + str.size(), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<float> parseFloat(const char* str) {
    float out = 0.0f;
    auto res = fast_float::from_chars(str, str + strlen(str), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<float> parseFloat(const cocos2d::CCString* str) {
    return parseFloat(str->getCString());
}

std::optional<double> parseDouble(std::string_view str) {
    double out = 0.0;
    auto res = fast_float::from_chars(str.data(), str.data() + str.size(), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<double> parseDouble(const char* str) {
    double out = 0.0;
    auto res = fast_float::from_chars(str, str + strlen(str), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<double> parseDouble(const cocos2d::CCString* str) {
    return parseDouble(str->getCString());
}

std::optional<int> parseInt(std::string_view str) {
    int out = 0;
    auto res = fast_float::from_chars(str.data(), str.data() + str.size(), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<int> parseInt(const char* str) {
    int out = 0;
    auto res = fast_float::from_chars(str, str + strlen(str), out);
    if (res.ec != std::errc{}) {
        return std::nullopt;
    }

    return out;
}

std::optional<int> parseInt(const cocos2d::CCString* str) {
    return parseInt(str->getCString());
}

}