#pragma once

#include <cstdint>
#include <cstddef>

namespace blaze {
    // xor a given buffer with a specific key
    void xor_u8(uint8_t* buffer, size_t size, uint8_t key);
}