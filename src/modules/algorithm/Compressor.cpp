#include "Compressor.hpp"
#include <libdeflate.h>
#include <Geode/utils/terminate.hpp>

using namespace geode::prelude;

namespace blaze {

Compressor::Compressor(int level) : m_mode(CompressionMode::Auto) {
    m_compressor = libdeflate_alloc_compressor(level);
    if (!m_compressor) {
        utils::terminate("failed to allocate compressor");
    }
}

Compressor::~Compressor() {
    libdeflate_free_compressor(m_compressor);
}

Compressor::Compressor(Compressor&& other) noexcept
    : m_compressor(other.m_compressor), m_mode(other.m_mode) {
    other.m_compressor = nullptr;
}

Compressor& Compressor::operator=(Compressor&& other) noexcept {
    if (this != &other) {
        libdeflate_free_compressor(m_compressor);
        m_compressor = other.m_compressor;
        m_mode = other.m_mode;
        other.m_compressor = nullptr;
    }
    return *this;
}

void Compressor::setMode(CompressionMode mode) {
    m_mode = mode;
}

size_t Compressor::getMaxCompressedSize(size_t rawSize) {
    switch (m_mode) {
        case CompressionMode::Deflate: return libdeflate_deflate_compress_bound(m_compressor, rawSize);
        case CompressionMode::Zlib: return libdeflate_zlib_compress_bound(m_compressor, rawSize);

        case CompressionMode::Auto:
        case CompressionMode::Gzip:
            return libdeflate_gzip_compress_bound(m_compressor, rawSize);
    }
}

size_t Compressor::compress(const void* input, size_t size, void* out, size_t outSize) {
    // if (m_multiThread) {
    //     return this->compressMultiThread(input, size, out, outSize);
    // }

    switch (m_mode) {
        case CompressionMode::Deflate: return libdeflate_deflate_compress(m_compressor, input, size, out, outSize);
        case CompressionMode::Zlib: return libdeflate_zlib_compress(m_compressor, input, size, out, outSize);

        case CompressionMode::Auto:
        case CompressionMode::Gzip:
            return libdeflate_gzip_compress(m_compressor, input, size, out, outSize);
    }
}

OwnedBuffer Compressor::compress(const void* input, size_t size) {
    auto maxSize = this->getMaxCompressedSize(size);
    auto data = std::make_unique_for_overwrite<uint8_t[]>(maxSize);
    size_t written = this->compress(input, size, data.get(), maxSize);

    return OwnedBuffer { std::move(data), written };
}


void Compressor::setMultiThreaded(bool multiThread) {
    m_multiThread = multiThread;
}

size_t Compressor::compressMultiThread(const void* input, size_t size, void* out, size_t outSize) {
    // libdeflate_deflate_compress()
    return 0;
}

CompressedChunk Compressor::compressChunkDeflate(const void* input, size_t size) {
    auto maxSize = this->getMaxCompressedSize(size);
    auto data = std::make_unique_for_overwrite<uint8_t[]>(maxSize);
    size_t written = libdeflate_deflate_compress(m_compressor, input, size, data.get(), maxSize);

    uint32_t adler32 = libdeflate_adler32(1, input, size);

    return CompressedChunk { OwnedBuffer {std::move(data), written}, size, adler32 };
}

}
