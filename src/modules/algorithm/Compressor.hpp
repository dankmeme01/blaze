#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <AsyncLoad/FileUtils.hpp>

struct libdeflate_compressor;
struct libdeflate_decompressor;

namespace blaze {

using AsyncLoad::OwnedBuffer;

enum class CompressionMode {
    Auto,
    Deflate,
    Zlib,
    Gzip
};

// TODO: multithread

struct CompressedChunk {
    OwnedBuffer data;
    size_t uncompressedSize;
    uint32_t adler32;
};

class Compressor {
public:
    Compressor(int level);
    ~Compressor();

    Compressor(const Compressor&) = delete;
    Compressor& operator=(const Compressor&) = delete;
    Compressor(Compressor&&) noexcept;
    Compressor& operator=(Compressor&&) noexcept;

    void setMode(CompressionMode mode);

    // Compute maximum size of the compressed output given the size of the input
    size_t getMaxCompressedSize(size_t rawSize);

    size_t compress(const void* input, size_t size, void* out, size_t outSize);
    OwnedBuffer compress(const void* input, size_t size);

    void setMultiThreaded(bool multiThread);

private:
    libdeflate_compressor* m_compressor;
    CompressionMode m_mode;
    bool m_multiThread = true;

    size_t compressMultiThread(const void* input, size_t size, void* out, size_t outSize);

    CompressedChunk compressChunkDeflate(const void* input, size_t size);
};

}
