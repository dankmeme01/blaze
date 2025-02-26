#include "memory_chunk.hpp"
#include "memory.hpp"

namespace blaze {

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


}