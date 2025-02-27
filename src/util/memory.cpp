#include "memory.hpp"

#ifdef _MSC_VER
#include <corecrt_malloc.h>
#endif

#include <Geode/loader/Log.hpp>
#include <new>

namespace blaze {

void* alignedMalloc(size_t size, size_t alignment) {
    auto ptr =
#ifdef _MSC_VER
    _aligned_malloc(size, alignment);
#else
    // std::aligned_alloc(alignment, size);
    nullptr;
    geode::log::warn("TODO implement");
#endif

    if (ptr) return ptr;

    if (auto h = std::get_new_handler()) {
        h();
    }

    static const std::bad_alloc exc;
    throw exc;
}

void alignedFree(void* data) {
#ifdef _MSC_VER
    _aligned_free(data);
#else
    std::free(data);
#endif
}

}