#include "util.hpp"

#include <cstdlib>

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

void unreachable() {
#ifdef __clang__
    __builtin_unreachable();
#else
    __assume(false);
#endif
}

OwnedMemoryChunk::~OwnedMemoryChunk() {
    std::free(data);
}

OwnedMemoryChunk::OwnedMemoryChunk(size_t size) : data(reinterpret_cast<uint8_t*>(std::malloc(size))), size(size) {
    if (!this->data) {
        throw std::bad_alloc();
    }
}

OwnedMemoryChunk::OwnedMemoryChunk(uint8_t* data, size_t size) : data(data), size(size) {}
OwnedMemoryChunk::OwnedMemoryChunk(std::unique_ptr<uint8_t[]> ptr, size_t size) : data(ptr.release()), size(size) {}
OwnedMemoryChunk::OwnedMemoryChunk() : data(nullptr), size(0) {}

std::pair<uint8_t*, size_t> OwnedMemoryChunk::release() {
    auto p = std::make_pair(data, size);

    this->data = nullptr;
    this->size = 0;

    return p;
}

void OwnedMemoryChunk::realloc(size_t newSize) {
    this->data = reinterpret_cast<uint8_t*>(std::realloc(data, newSize));
    if (!this->data) {
        throw std::bad_alloc();
    }

    this->size = newSize;
}

OwnedMemoryChunk::OwnedMemoryChunk(OwnedMemoryChunk&& other) {
    std::tie(this->data, this->size) = other.release();
}

OwnedMemoryChunk& OwnedMemoryChunk::operator=(OwnedMemoryChunk&& other) {
    if (this != &other) {
        std::tie(this->data, this->size) = other.release();
    }

    return *this;
}

OwnedMemoryChunk::operator bool() {
    return this->data != nullptr;
}

}