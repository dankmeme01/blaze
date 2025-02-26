#pragma once

#include <cstdint>
#include <string_view>

// Compile- and runtime FNV-1a hash for strings

namespace blaze {

constexpr uint32_t FNV_OFFSET_BASIS = 2166136261u;
constexpr uint32_t FNV_PRIME = 16777619u;

constexpr uint32_t _fnv1a_hash(const char *str, uint32_t hash = FNV_OFFSET_BASIS) {
    return (*str == '\0') ? hash : _fnv1a_hash(str + 1, (hash ^ static_cast<uint8_t>(*str)) * FNV_PRIME);
}

#define BLAZE_STRING_HASH(x) (::blaze::_fnv1a_hash(x))

uint32_t hashStringRuntime(const char* str);
uint32_t hashStringRuntime(std::string_view str);

}