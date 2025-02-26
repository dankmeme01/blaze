#include "string.hpp"
#include <fast_float/fast_float.h>

using namespace geode::prelude;

namespace blaze {

static void _dbgcheck(const fast_float::from_chars_result& result) {
#ifdef BLAZE_DEBUG
    if (result.ec != std::errc{}) {
        log::warn("float parsing error: {}", (int) result.ec);
        log::warn("input: {}", result.ptr);
    }
#endif
}

float parseFloat(std::string_view str) {
    float out = 0.0f;
    _dbgcheck(fast_float::from_chars(str.data(), str.data() + str.size(), out));
    return out;
}

float parseFloat(const char* str) {
    float out = 0.0f;
    _dbgcheck(fast_float::from_chars(str, str + strlen(str), out));
    return out;
}

float parseFloat(const cocos2d::CCString* str) {
    return parseFloat(str->getCString());
}

double parseDouble(std::string_view str) {
    double out = 0.0;
    _dbgcheck(fast_float::from_chars(str.data(), str.data() + str.size(), out));
    return out;
}

double parseDouble(const char* str) {
    double out = 0.0;
    _dbgcheck(fast_float::from_chars(str, str + strlen(str), out));
    return out;
}

double parseDouble(const cocos2d::CCString* str) {
    return parseDouble(str->getCString());
}

int parseInt(std::string_view str) {
    int out = 0;
    _dbgcheck(fast_float::from_chars(str.data(), str.data() + str.size(), out));
    return out;
}

int parseInt(const char* str) {
    int out = 0;
    _dbgcheck(fast_float::from_chars(str, str + strlen(str), out));
    return out;
}

int parseInt(const cocos2d::CCString* str) {
    return parseInt(str->getCString());
}

}