#include "base64.hpp"

#include <util.hpp>
#include <simdutf.h>

using namespace geode::prelude;

namespace blaze::base64 {
    // Functions that are available on any implementation

    std::vector<char> encode(const uint8_t* data, size_t size, bool urlsafe) {
        std::vector<char> vec(encodedLen(size));

        size_t outlen = encode(data, size, vec.data(), urlsafe);
        vec.resize(outlen);

        return vec;
    }

    std::string encodeToString(const uint8_t* data, size_t size, bool urlsafe) {
        std::string out(encodedLen(size), '\0');

        size_t outlen = encode(data, size, out.data(), urlsafe);
        out.resize(outlen);

        return out;
    }

    size_t decode(std::string_view data, uint8_t* outBuf, bool urlsafe) {
        return decode(data.data(), data.size(), outBuf, urlsafe);
    }

    std::vector<uint8_t> decode(const char* data, size_t size, bool urlsafe) {
        std::vector<uint8_t> out(decodedLen(data, size));

        size_t encsize = decode(data, size, out.data(), urlsafe);
        out.resize(encsize);

        return out;
    }

    std::vector<uint8_t> decode(std::string_view data, bool urlsafe) {
        return decode(data.data(), data.size(), urlsafe);
    }

    /* forwarders */

    size_t encode(const uint8_t* data, size_t size, char* outBuf, bool urlsafe) {
        return simdutf::encode(data, size, outBuf, urlsafe);
    }

    size_t encodedLen(size_t rawLen, bool urlsafe) {
        return simdutf::encodedLen(rawLen, urlsafe);
    }

    size_t decode(const char* data, size_t size, uint8_t* outBuf, bool urlsafe) {
        return simdutf::decode(data, size, outBuf, urlsafe);
    }

    size_t decodedLen(const char* data, size_t dataLen) {
        return simdutf::decodedLen(data, dataLen);
    }

    namespace simdutf {
        static ::simdutf::base64_options getFlags(bool urlsafe) {
            return urlsafe ? ::simdutf::base64_url : ::simdutf::base64_default_no_padding;
        }

        static bool isWhitespace(char c) {
            // ignored chars
            return c == '\n' || c == '\r' || c == ' ' || c == '\t' || c == '\0';
        }

        size_t encode(const uint8_t* data, size_t size, char* outBuf, bool urlsafe) {
            return ::simdutf::binary_to_base64(reinterpret_cast<const char*>(data), size, outBuf, getFlags(urlsafe));
        }

        size_t encodedLen(size_t rawLen, bool urlsafe) {
            return ::simdutf::base64_length_from_binary(rawLen, getFlags(urlsafe));
        }

        size_t decode(const char* data, size_t size, uint8_t* outBuf, bool urlsafe) {
            // strip padding and bad chars
            while (data[size - 1] == '=' || isWhitespace(data[size - 1])) size--;

            auto result = ::simdutf::base64_to_binary(data, size, reinterpret_cast<char*>(outBuf), getFlags(urlsafe));
            if (result.error) {
                log::warn("Base64 decode error after {} chars (on {}): {}", result.count, data[result.count], (int)result.error);
                return 0;
            }

            return result.count;
        }

        size_t decodedLen(const char* data, size_t dataLen) {
            return ::simdutf::maximal_binary_length_from_base64(data, dataLen);
        }
    }

}
