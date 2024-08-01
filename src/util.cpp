#include "util.hpp"

#include <cstdlib>

namespace blaze {

void* alignedMalloc(size_t size, size_t alignment) {
    auto ptr =
#ifdef _MSC_VER
    _aligned_malloc(size, alignment);
#else
    std::aligned_alloc(alignment, size);
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

void unreachable() {
#ifdef __clang__
    __builtin_unreachable();
#else
    __assume(false);
#endif
}


}