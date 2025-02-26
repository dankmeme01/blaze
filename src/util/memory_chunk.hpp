#pragma once

namespace blaze {
    // kinda like unique_ptr<uint8_t[]> but with specified size
    struct OwnedMemoryChunk {
        uint8_t* data = nullptr;
        size_t size;

        ~OwnedMemoryChunk();

        OwnedMemoryChunk(size_t size);

        // Note that this class takes ownership of the passed pointer!
        OwnedMemoryChunk(uint8_t* data, size_t size);
        OwnedMemoryChunk(std::unique_ptr<uint8_t[]> ptr, size_t size);
        OwnedMemoryChunk();

        std::pair<uint8_t*, size_t> release();

        void realloc(size_t newSize);

        OwnedMemoryChunk(const OwnedMemoryChunk&) = delete;
        OwnedMemoryChunk& operator=(const OwnedMemoryChunk&) = delete;

        OwnedMemoryChunk(OwnedMemoryChunk&& other);

        OwnedMemoryChunk& operator=(OwnedMemoryChunk&& other);

        explicit operator bool();
    };

    struct AlignedMemoryChunk : OwnedMemoryChunk {
        size_t align;

        ~AlignedMemoryChunk();
        AlignedMemoryChunk(size_t size, size_t alignment);
        AlignedMemoryChunk();
        // Note that this class takes ownership of the passed pointer!
        AlignedMemoryChunk(uint8_t* data, size_t size, size_t alignment);

        using OwnedMemoryChunk::release;

        void realloc(size_t newSize);

        AlignedMemoryChunk(const AlignedMemoryChunk&) = delete;
        AlignedMemoryChunk& operator=(const AlignedMemoryChunk&) = delete;

        AlignedMemoryChunk(AlignedMemoryChunk&& other);

        AlignedMemoryChunk& operator=(AlignedMemoryChunk&& other);

        using OwnedMemoryChunk::operator bool;
    };
}