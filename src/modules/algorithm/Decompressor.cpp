#include "Decompressor.hpp"
#include <libdeflate.h>
#include <Geode/utils/terminate.hpp>

using namespace geode::prelude;

namespace blaze {

static constexpr int ERR_MALFORMED = 12345;

Decompressor::Decompressor() : m_mode(DecompressionMode::Auto) {
    m_decompressor = libdeflate_alloc_decompressor();
    if (!m_decompressor) {
        utils::terminate("failed to allocate compressor");
    }
}

Decompressor::~Decompressor() {
    libdeflate_free_decompressor(m_decompressor);
}

void Decompressor::setMode(DecompressionMode mode) {
    m_mode = mode;
}

void Decompressor::setModeAuto(const void* input, size_t size) {
    if (size < 2) {
        return; // too small to determine
    }

    const uint8_t* data = (const uint8_t*)input;
    if (data[0] == 0x1f && data[1] == 0x8b) {
        this->setMode(DecompressionMode::Gzip);
    } else if (data[0] == 0x78 && (data[1] == 0x01 || data[1] == 0x9c || data[1] == 0xda || data[1] == 0x5e)) {
        this->setMode(DecompressionMode::Zlib);
    } else {
        log::warn("Failed to determine compression mode, using default.");

        if (size >= 6) {
            log::warn("First few bytes of the header: {:X} {:X} {:X} {:X} {:X} {:X}", data[0], data[1], data[2], data[3], data[4], data[5]);
        }

        this->setMode(DecompressionMode::Gzip);
    }
}

Result<size_t, int> Decompressor::decompress(const void* input, size_t size, void* out, size_t outSize) {
    libdeflate_result res;

    if (m_mode == DecompressionMode::Auto) {
        this->setModeAuto(input, size);
    }

    size_t writtenSize = 0;
    switch (m_mode) {
        case DecompressionMode::Deflate: res = libdeflate_deflate_decompress(m_decompressor, input, size, out, outSize, &writtenSize); break;
        case DecompressionMode::Gzip: res = libdeflate_gzip_decompress(m_decompressor, input, size, out, outSize, &writtenSize); break;
        case DecompressionMode::Zlib: res = libdeflate_zlib_decompress(m_decompressor, input, size, out, outSize, &writtenSize); break;

        case DecompressionMode::Auto:
            return Err(ERR_MALFORMED);
    }

    switch (res) {
        case LIBDEFLATE_SUCCESS: return Ok(writtenSize);
        default: return Err((int)res);
    }
}

Result<OwnedBuffer> Decompressor::decompress(const void* input, size_t size) {
    // heuristic
    size_t curSize = size * 3;
    auto buf = std::make_unique_for_overwrite<uint8_t[]>(curSize);

    while (true) {
        auto res = this->decompress(input, size, buf.get(), curSize);
        if (res) {
            return Ok(OwnedBuffer{std::move(buf), res.unwrap()});
        }

        auto err = res.unwrapErr();
        switch (err) {
            case LIBDEFLATE_INSUFFICIENT_SPACE: {
                curSize *= 2;
                buf = std::make_unique_for_overwrite<uint8_t[]>(curSize);
                continue;
            } break;

            case LIBDEFLATE_BAD_DATA:
                return Err("Decompression failed: Malformed data");
            case LIBDEFLATE_SHORT_OUTPUT:
                return Err("Decompression failed: Short output");
            default:
                return Err("Decompression failed: unknown error {}", err);
        }
    }
}

Result<std::string> Decompressor::decompressToString(const void* input, size_t size) {
    // heuristic
    size_t curSize = size * 3;
    std::string buf;
    buf.resize(curSize);

    while (true) {
        auto res = this->decompress(input, size, buf.data(), curSize);
        if (res) {
            buf.resize(res.unwrap());
            return Ok(std::move(buf));
        }

        auto err = res.unwrapErr();
        switch (err) {
            case LIBDEFLATE_INSUFFICIENT_SPACE: {
                curSize *= 2;
                buf.resize(curSize);
                continue;
            } break;

            case LIBDEFLATE_BAD_DATA:
                return Err("Decompression failed: Malformed data");
            case LIBDEFLATE_SHORT_OUTPUT:
                return Err("Decompression failed: Short output");
            default:
                return Err("Decompression failed: unknown error {}", err);
        }
    }
}

void Decompressor::setMultiThreaded(bool multiThread) {
    m_multiThread = multiThread;
}

}
