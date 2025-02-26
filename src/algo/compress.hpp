#pragma once

#include <stddef.h>
#include <stdint.h>
#include <util.hpp>
#include <util/memory_chunk.hpp>

struct libdeflate_compressor;
struct libdeflate_decompressor;

namespace blaze {
    enum class CompressionMode {
        Deflate,
        Zlib,
        Gzip
    };

    class Compressor {
    public:
        Compressor(int level);
        ~Compressor();
        void setMode(CompressionMode mode);

        // Compute maximum size of the compressed output given the size of the input
        size_t getMaxCompressedSize(size_t rawSize);

        size_t compress(const void* input, size_t size, void* out, size_t outSize);
        OwnedMemoryChunk compressToChunk(const void* input, size_t size);
        std::vector<uint8_t> compress(const void* input, size_t size);

    private:
        libdeflate_compressor* compressor;
        CompressionMode mode;
    };

    class Decompressor {
    public:
        Decompressor();
        ~Decompressor();
        void setMode(CompressionMode mode);

        geode::Result<size_t, int> decompress(const void* input, size_t size, void* out, size_t outSize, size_t& writtenSize);
        geode::Result<OwnedMemoryChunk> decompressToChunk(const void* input, size_t size);
        geode::Result<std::vector<uint8_t>> decompress(const void* input, size_t size);
        geode::Result<std::string> decompressToString(const void* input, size_t size);

    private:
        libdeflate_decompressor* decompressor;
        CompressionMode mode;
    };

    std::vector<uint8_t> compress(const uint8_t* data, size_t size);
}
