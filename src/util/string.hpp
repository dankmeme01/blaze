#pragma once

#include <string_view>
#include <cocos2d.h>

namespace blaze {

std::optional<float> parseFloat(std::string_view str);
std::optional<float> parseFloat(const char* str);
std::optional<float> parseFloat(const cocos2d::CCString* str);

std::optional<double> parseDouble(std::string_view str);
std::optional<double> parseDouble(const char* str);
std::optional<double> parseDouble(const cocos2d::CCString* str);

std::optional<int> parseInt(std::string_view str);
std::optional<int> parseInt(const char* str);
std::optional<int> parseInt(const cocos2d::CCString* str);

}