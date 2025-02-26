#pragma once

#include <string_view>
#include <cocos2d.h>

namespace blaze {

float parseFloat(std::string_view str);
float parseFloat(const char* str);
float parseFloat(const cocos2d::CCString* str);

double parseDouble(std::string_view str);
double parseDouble(const char* str);
double parseDouble(const cocos2d::CCString* str);

int parseInt(std::string_view str);
int parseInt(const char* str);
int parseInt(const cocos2d::CCString* str);

}