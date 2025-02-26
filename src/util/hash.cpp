#include "hash.hpp"

namespace blaze {

uint32_t hashStringRuntime(const char* str) {
    uint32_t hash = FNV_OFFSET_BASIS;

    while (*str) {
        hash = ((hash ^ static_cast<uint8_t>(*str++)) * FNV_PRIME);
    }

    return hash;
}

uint32_t hashStringRuntime(std::string_view str) {
    uint32_t hash = FNV_OFFSET_BASIS;

    for (char c : str) {
        hash = ((hash ^ static_cast<uint8_t>(c)) * FNV_PRIME);
    }

    return hash;
}

}