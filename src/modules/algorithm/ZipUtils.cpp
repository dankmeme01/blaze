#include "Compressor.hpp"
#include "Decompressor.hpp"
#include "AlgorithmModule.hpp"
#include "base64.hpp"
#include "xor.hpp"
#include <Geode/modify/ZipUtils.hpp>
#include <Geode/utils/base64.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(ZipUtils) {
    static void onModify(auto& self) {
        AlgorithmModule::get().addHooks(self,
            "cocos2d::ZipUtils::ccDeflateMemory",
            "cocos2d::ZipUtils::ccInflateMemory",
            "cocos2d::ZipUtils::ccInflateMemoryWithHint",
            "cocos2d::ZipUtils::compressString",
            "cocos2d::ZipUtils::decompressString",
            "cocos2d::ZipUtils::decompressString2"
        );
    }

    $override
    static int ccDeflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        Compressor c{1};
        c.setMode(CompressionMode::Gzip);
        auto buf = c.compress(input, size);

        *outp = buf.data.release();
        return buf.size;
    }

    $override
    static int ccInflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        Decompressor dec;
        auto res = dec.decompress(input, size);

        if (!res) {
            log::warn("ccDeflateMemory failed, calling original: {}", res.unwrapErr());
            return ZipUtils::ccInflateMemory(input, size, outp);
        }

        auto buf = std::move(res).unwrap();

        *outp = buf.data.release();
        return buf.size;
    }

    $override
    static int ccInflateMemoryWithHint(unsigned char* input, unsigned int size, unsigned char** outp, unsigned int *outLength, unsigned int hint) {
        return ccInflateMemory(input, size, outp);
    }

    $override
    static gd::string compressString(gd::string const& data, bool encrypt, int key) {
        Compressor c{1};
        c.setMode(CompressionMode::Gzip);

        auto buf = c.compress(data.data(), data.size());

        auto encoded = base64::encode(buf.span());

        if (encrypt) {
            encryptDecryptImpl(encoded.data(), encoded.size(), key);
        }

#ifdef GEODE_IS_ANDROID
        return gd::string{encoded.begin(), encoded.end()};
#else
        return encoded;
#endif
    }

    static void encryptDecryptImpl(void* data, size_t size, int key) {
        blaze::xor_u8_inplace(static_cast<uint8_t*>(data), size, static_cast<uint8_t>(key));
    }

    ///// string decompression /////

    $override
    static gd::string decompressString(gd::string const& input, bool encrypted, int key) {
        if (input.empty()) return "";

        std::vector<uint8_t> rawData;
        if (!encrypted) {
            rawData = base64::decode(input).unwrapOrDefault();
        } else {
            // decrypt into a temporary buffer, then decode
            auto buf = std::make_unique_for_overwrite<char[]>(input.size());
            blaze::xor_u8((uint8_t*)input.data(), (uint8_t*)buf.get(), input.size(), static_cast<uint8_t>(key));

            rawData = base64::decode({buf.get(), input.size()}).unwrapOrDefault();
        }

        if (rawData.empty()) {
            log::warn("decompressString fail 1");
            // if failed, try to fall back to original implementation, "just in case"
            return ZipUtils::decompressString(input, encrypted, key);
        }

        Decompressor d;
        auto result = d.decompressToString(rawData.data(), rawData.size());

        if (result.isOk()) {
            // success!
#ifdef GEODE_IS_ANDROID
            return gd::string(std::move(result).unwrap());
#else
            return std::move(result).unwrap();
#endif
        }

        log::warn("decompressString fail 2: {}", result.unwrapErr());

        // if failed, try to fall back to original implementation, "just in case"
        return ZipUtils::decompressString(input, encrypted, key);
    }

    $override
    static gd::string decompressString2(unsigned char* data, bool encrypted, int size, int key) {
        if (!data || size < 1) return "";

        if (encrypted) {
            // decrypt the data right into the original buffer
            encryptDecryptImpl(data, size, key);
        }

        std::string_view input{(const char*)data, (size_t)size};
        auto rawData = base64::decode(input).unwrapOrDefault();
        if (rawData.empty()) {
            log::warn("decompressString2 fail 1");
            // if failed, try to fall back to original implementation
            if (encrypted) {
                encryptDecryptImpl(data, size, key);
            }

            return ZipUtils::decompressString2(data, encrypted, size, key);
        }

        Decompressor d;
        auto result = d.decompressToString(rawData.data(), rawData.size());

        if (result.isOk()) {
            auto retval = std::move(result.unwrap());
            // success!
#ifdef GEODE_IS_ANDROID
            return gd::string(retval);
#else
            return retval;
#endif
        }

        log::warn("decompressString2 fail 2: {}", result.unwrapErr());

        // if failed, try to fall back to original implementation
        if (encrypted) {
            encryptDecryptImpl(data, size, key);
        }

        return ZipUtils::decompressString2(data, encrypted, size, key);
    }
};

}
