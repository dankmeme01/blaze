#pragma once

#include <stdint.h>

namespace blaze::base64 {
    // Encodes base64 data to a char vector.
    std::vector<char> encode(const uint8_t* data, size_t size, bool urlsafe = false);

    // Encodes base64 data to a string.
    std::string encodeToString(const uint8_t* data, size_t size, bool urlsafe = false);

    // Decodes base64 data into the given buffer. Returns 0 on failure.
    size_t decode(std::string_view data, uint8_t* outBuf, bool urlsafe = false);

    // Decodes base64 data to a byte vector.
    std::vector<uint8_t> decode(const char* data, size_t size, bool urlsafe = false);
    // Decodes base64 data to a byte vector.
    std::vector<uint8_t> decode(std::string_view data, bool urlsafe = false);

    // Encodes base64 data into the given buffer. Returns 0 on failure.
    size_t encode(const uint8_t* data, size_t size, char* outBuf, bool urlsafe = false);

    // Returns the size of the buffer needed to hold the encoded data
    size_t encodedLen(size_t rawLen, bool urlsafe = false);

    // Decodes base64 data into the given buffer. Returns 0 on failure.
    size_t decode(const char* data, size_t size, uint8_t* outBuf, bool urlsafe = false);

    // Returns the size of the buffer needed to hold the decoded data
    size_t decodedLen(const char* data, size_t dataLen);
}

namespace blaze::base64::simdutf {
    // Encodes base64 data into the given buffer. Returns 0 on failure.
    size_t encode(const uint8_t* data, size_t size, char* outBuf, bool urlsafe = false);

    // Returns the size of the buffer needed to hold the encoded data
    size_t encodedLen(size_t rawLen, bool urlsafe = false);

    // Decodes base64 data into the given buffer. Returns 0 on failure.
    size_t decode(const char* data, size_t size, uint8_t* outBuf, bool urlsafe = false);

    // Returns the size of the buffer needed to hold the decoded data
    size_t decodedLen(const char* data, size_t dataLen);
}
