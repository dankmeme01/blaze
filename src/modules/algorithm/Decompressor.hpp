#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <AsyncLoad/FileUtils.hpp>

struct libdeflate_compressor;
struct libdeflate_decompressor;

namespace blaze {

using AsyncLoad::OwnedBuffer;

enum class DecompressionMode {
    Auto,
    Deflate,
    Zlib,
    Gzip
};

// TODO: multithread

class Decompressor {
public:
    Decompressor();
    ~Decompressor();

    Decompressor(const Decompressor&) = delete;
    Decompressor& operator=(const Decompressor&) = delete;
    Decompressor(Decompressor&&) noexcept;
    Decompressor& operator=(Decompressor&&) noexcept;

    void setMode(DecompressionMode mode);

    // Reads the buffer, attempts to pick the correct decompression mode
    // Optional, when calling `decompress` without explicitly setting a mode, this will be called anyway to determine the mode.
    void setModeAuto(const void* input, size_t size);

    geode::Result<size_t, int> decompress(const void* input, size_t size, void* out, size_t outSize);
    geode::Result<OwnedBuffer> decompress(const void* input, size_t size);
    geode::Result<std::string> decompressToString(const void* input, size_t size);

    void setMultiThreaded(bool multiThread);

private:
    libdeflate_decompressor* m_decompressor;
    DecompressionMode m_mode;
    bool m_multiThread = true;

};

}
