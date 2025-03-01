#include "compress.hpp"

#include <libdeflate.h>
#include <Geode/loader/Log.hpp>
#include <Geode/Result.hpp>
#include <Geode/Prelude.hpp>

using namespace geode::prelude;

namespace blaze {
    constexpr auto DEFAULT_COMPRESSION = CompressionMode::Gzip;
    constexpr auto DEFAULT_DECOMPRESSION = CompressionMode::Gzip;

    Compressor::Compressor(int level) : mode(DEFAULT_COMPRESSION) {
        this->compressor = libdeflate_alloc_compressor(level);

        if (!compressor) {
            throw std::runtime_error("Failed to create compressor");
        }
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

    Decompressor::Decompressor() : mode(DEFAULT_DECOMPRESSION) {
        this->decompressor = libdeflate_alloc_decompressor();

        if (!this->decompressor) {
            throw std::runtime_error("Failed to create decompressor");
        }
    }

    Decompressor::~Decompressor() {
        libdeflate_free_decompressor(this->decompressor);
    }

    void Decompressor::setMode(CompressionMode mode) {
        this->mode = mode;
    }

    void Decompressor::setModeAuto(const void* input, size_t size) {
        if (size < 2) {
            return; // too small to determine
        }

        const uint8_t* data = (const uint8_t*)input;
        if (data[0] == 0x1f && data[1] == 0x8b) {
            this->setMode(CompressionMode::Gzip);
        } else if (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x9c || data[1] == 0xda || data[1] == 0x5e)) {
            this->setMode(CompressionMode::Zlib);
        } else {
            log::warn("Failed to determine compression mode, using default.");

            if (size >= 6) {
                log::warn("First few bytes of the header: {:X} {:X} {:X} {:X} {:X} {:X}", data[0], data[1], data[2], data[3], data[4], data[5]);
            }

            this->setMode(DEFAULT_DECOMPRESSION);
        }
    }

    CompressionMode Decompressor::getMode() const {
        return mode;
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
        OwnedMemoryChunk chunk(size * 3);

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
}
