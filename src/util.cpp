#include "util.hpp"

#include <cstdlib>

#ifdef GEODE_IS_WINDOWS
# include <psapi.h>
# include <Windows.h>
#endif

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

AlignedMemoryChunk::~AlignedMemoryChunk() {
    blaze::alignedFree(data);
    data = nullptr;
}

static size_t alignUp(size_t size, size_t align) {
    return (size + align - 1) & ~(align - 1);
}

static bool isAligned(size_t number, size_t align) {
    return (number & (align - 1)) == 0;
}

AlignedMemoryChunk::AlignedMemoryChunk() : OwnedMemoryChunk() {}
AlignedMemoryChunk::AlignedMemoryChunk(size_t size, size_t align) : OwnedMemoryChunk((uint8_t*)blaze::alignedMalloc(size, align), size), align(align) {}

void AlignedMemoryChunk::realloc(size_t newSize) {
    auto outptr = blaze::alignedMalloc(newSize, align);

    if (data && outptr) {
        std::memcpy(outptr, data, std::min(size, newSize));
    }

    blaze::alignedFree(data);

    data = reinterpret_cast<uint8_t*>(outptr);
}

AlignedMemoryChunk::AlignedMemoryChunk(uint8_t* data, size_t size, size_t alignment) : OwnedMemoryChunk(data, size) {
    if (!isAligned(size, alignment)) {
        throw std::runtime_error("Size is not aligned to alignment");
    }
}

AlignedMemoryChunk::AlignedMemoryChunk(AlignedMemoryChunk&& other) {
    std::tie(this->data, this->size) = other.release();
    this->align = other.align;
}

AlignedMemoryChunk& AlignedMemoryChunk::operator=(AlignedMemoryChunk&& other) {
    if (this != &other) {
        std::tie(this->data, this->size) = other.release();
        this->align = other.align;
    }

    return *this;
}

// Thanks prevter!
// https://github.com/Prevter/vapor/blob/main/src/main.cpp
size_t imageSize() {
    static size_t size = []() -> size_t {
#ifdef GEODE_IS_WINDOWS
        auto handle = GetModuleHandle(nullptr);
        if (!handle) return 0;

        MODULEINFO info;
        if (!GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info)))
            return 0;

        return info.SizeOfImage;
#elif defined(GEODE_IS_INTEL_MAC)
        return 0x980000;
#elif defined(GEODE_IS_ARM_MAC)
        return 0x8B0000;
#else
        return 0;
#endif
    }();

    return size;
}

size_t cocosImageSize() {
    static size_t size = []() -> size_t {
#ifdef GEODE_IS_WINDOWS
        auto handle = GetModuleHandle("libcocos2d.dll");
        if (!handle) return 0;

        MODULEINFO info;
        if (!GetModuleInformation(GetCurrentProcess(), handle, &info, sizeof(info)))
            return 0;

        return info.SizeOfImage;
#elif defined(GEODE_IS_INTEL_MAC)
        return 0x980000;
#elif defined(GEODE_IS_ARM_MAC)
        return 0x8B0000;
#else
        return imageSize();
#endif
    }();

    return size;

}

}