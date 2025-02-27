#include "base64.hpp"

#include <util.hpp>
#include <simdutf.h>

#include <Geode/loader/Log.hpp>
#include <Geode/Prelude.hpp>

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

    static size_t fixEncodedBase64(const char* data, size_t size) {
        for (size_t i = std::max((size_t)0, size - 8); i < size; i++) {
            if (data[i] == '=' || data[i] == '\0') {
                return i;
            }
        }

        return size;
    }

    static size_t breakEncodedBase64(char* outBuf, size_t size) {
        // i believe cocos base64 decoder needs a null byte so /shrug
        // i have spent so much time fighting with that thing i cant be bothered lol
        outBuf[size] = '=';
        outBuf[size + 1] = '\0';

        return size + 2;
    }

    namespace simdutf {
        static ::simdutf::base64_options getFlags(bool urlsafe) {
            return urlsafe ? ::simdutf::base64_url : ::simdutf::base64_default_no_padding;
        }

        size_t encode(const uint8_t* data, size_t size, char* outBuf, bool urlsafe) {
            size_t out = ::simdutf::binary_to_base64(reinterpret_cast<const char*>(data), size, outBuf, getFlags(urlsafe));
            out = breakEncodedBase64(outBuf, out);
            return out;
        }

        size_t encodedLen(size_t rawLen, bool urlsafe) {
            return ::simdutf::base64_length_from_binary(rawLen, getFlags(urlsafe)) + 2; // the +2 is from `breakEncodedBase64`
        }

        size_t decode(const char* data, size_t size, uint8_t* outBuf, bool urlsafe) {
            // a weird peculiarity of gd/cocos is that data can have some trailing garbage at the end, we want to avoid decompressing that
            size = fixEncodedBase64(data, size);

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
