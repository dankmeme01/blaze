#include "compress.hpp"

#include <libdeflate.h>

using namespace geode::prelude;

namespace blaze {
    Compressor::Compressor(int level) : mode(CompressionMode::Gzip) {
        this->compressor = libdeflate_alloc_compressor(level);
    }

    Compressor::~Compressor() {
        libdeflate_free_compressor(this->compressor);
    }

    void Compressor::setMode(CompressionMode mode) {
        this->mode = mode;
    }

    size_t Compressor::getMaxCompressedSize(size_t rawSize) {
        switch (mode) {
            case CompressionMode::Deflate: return libdeflate_deflate_compress_bound(this->compressor, rawSize);
            case CompressionMode::Gzip: return libdeflate_gzip_compress_bound(this->compressor, rawSize);
            case CompressionMode::Zlib: return libdeflate_zlib_compress_bound(this->compressor, rawSize);
        }
    }

    size_t Compressor::compress(const void* input, size_t size, void* out, size_t outSize) {
        switch (mode) {
            case CompressionMode::Deflate: return libdeflate_deflate_compress(compressor, input, size, out, outSize);
            case CompressionMode::Gzip: return libdeflate_gzip_compress(compressor, input, size, out, outSize);
            case CompressionMode::Zlib: return libdeflate_zlib_compress(compressor, input, size, out, outSize);
        }
    }

    OwnedMemoryChunk Compressor::compressToChunk(const void* input, size_t size) {
        OwnedMemoryChunk chunk(this->getMaxCompressedSize(size));

        size_t writtenBytes = this->compress(input, size, chunk.data, chunk.size);
        chunk.size = writtenBytes;

        return chunk;
    }

    std::vector<uint8_t> Compressor::compress(const void* input, size_t size) {
        std::vector<uint8_t> out;

        out.resize(this->getMaxCompressedSize(size));
        size_t writtenBytes = this->compress(input, size, out.data(), out.size());
        out.resize(writtenBytes);

        return out;
    }

    Decompressor::Decompressor() : mode(CompressionMode::Gzip) {
        this->decompressor = libdeflate_alloc_decompressor();
    }

    Decompressor::~Decompressor() {
        libdeflate_free_decompressor(this->decompressor);
    }

    void Decompressor::setMode(CompressionMode mode) {
        this->mode = mode;
    }

    Result<size_t, int> Decompressor::decompress(const void* input, size_t size, void* out, size_t outSize, size_t& writtenSize) {
        libdeflate_result res;

        switch (mode) {
            case CompressionMode::Deflate: res = libdeflate_deflate_decompress(decompressor, input, size, out, outSize, &writtenSize); break;
            case CompressionMode::Gzip: res = libdeflate_gzip_decompress(decompressor, input, size, out, outSize, &writtenSize); break;
            case CompressionMode::Zlib: res = libdeflate_zlib_decompress(decompressor, input, size, out, outSize, &writtenSize); break;
        }


        switch (res) {
            case LIBDEFLATE_SUCCESS: return Ok(writtenSize);
            default: return Err((int)res);
        }
    }

    Result<OwnedMemoryChunk> Decompressor::decompressToChunk(const void* input, size_t size) {
        OwnedMemoryChunk chunk(size * 2);

        size_t writtenSize;

        auto result1 = this->decompress(input, size, chunk.data, chunk.size, writtenSize);
        if (result1.isOk()) {
            chunk.size = writtenSize;
            return Ok(std::move(chunk));
        }

        libdeflate_result res = (libdeflate_result)result1.unwrapErr();

        // double the size until it can fit
        while (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
            log::debug("Reallocating {} to {}", chunk.size, chunk.size * 2);

            chunk.realloc(chunk.size * 2);

            auto result = this->decompress(input, size, chunk.data, chunk.size, writtenSize);
            if (result.isOk()) {
                chunk.size = writtenSize;
                return Ok(std::move(chunk));
            }

            res = (libdeflate_result)result.unwrapErr();
        };

        switch (res) {
            case LIBDEFLATE_BAD_DATA: return Err("compressed data was invalid");
            case LIBDEFLATE_SHORT_OUTPUT: return Err("short buffer output");
            default: blaze::unreachable();
        }
    }

    Result<std::vector<uint8_t>> Decompressor::decompress(const void* input, size_t size) {
        std::vector<uint8_t> chunk(size * 2);

        size_t writtenSize;

        auto result1 = this->decompress(input, size, chunk.data(), chunk.size(), writtenSize);
        if (result1.isOk()) {
            chunk.resize(writtenSize);
            return Ok(std::move(chunk));
        }

        libdeflate_result res = (libdeflate_result)result1.unwrapErr();

        // double the size until it can fit
        while (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
            log::debug("Reallocating {} to {}", chunk.size(), chunk.size() * 2);

            chunk.resize(chunk.size() * 2);

            auto result = this->decompress(input, size, chunk.data(), chunk.size(), writtenSize);
            if (result.isOk()) {
                chunk.resize(writtenSize);
                return Ok(std::move(chunk));
            }

            res = (libdeflate_result)result.unwrapErr();
        };

        switch (res) {
            case LIBDEFLATE_BAD_DATA: return Err("compressed data was invalid");
            case LIBDEFLATE_SHORT_OUTPUT: return Err("short buffer output");
            default: blaze::unreachable();
        }
    }

    Result<std::string> Decompressor::decompressToString(const void* input, size_t size) {
        std::string chunk(size * 2, '\0');

        size_t writtenSize;

        auto result1 = this->decompress(input, size, chunk.data(), size * 32, writtenSize);
        if (result1.isOk()) {
            chunk.resize(writtenSize);
            return Ok(std::move(chunk));
        }

        libdeflate_result res = (libdeflate_result)result1.unwrapErr();

        // double the size until it can fit
        while (res == LIBDEFLATE_INSUFFICIENT_SPACE) {
            log::debug("Reallocating {} to {}", chunk.size(), chunk.size() * 2);

            chunk.resize(chunk.size() * 2);

            auto result = this->decompress(input, size, chunk.data(), chunk.size(), writtenSize);
            if (result.isOk()) {
                chunk.resize(writtenSize);
                return Ok(std::move(chunk));
            }

            res = (libdeflate_result)result.unwrapErr();
        };

        switch (res) {
            case LIBDEFLATE_BAD_DATA: return Err("compressed data was invalid");
            case LIBDEFLATE_SHORT_OUTPUT: return Err("short buffer output");
            default: blaze::unreachable();
        }
    }
}

$execute {
    auto* compressor = libdeflate_alloc_compressor(3);
    auto outBytes = libdeflate_zlib_compress_bound(compressor, 3);
}