#pragma once

namespace blaze {
    // Allocates aligned memory on heap, throws std::bad_alloc on failure. Must be freed with `alignedFree`
    void* alignedMalloc(size_t size, size_t alignment);
    void alignedFree(void* data);
}