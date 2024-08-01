#include <Geode/Geode.hpp>
#include <Geode/modify/ZipUtils.hpp>

#include <algo/base64.hpp>
#include <algo/xor.hpp>
#include <util.hpp>

using namespace geode::prelude;

class $modify(ZipUtils) {
    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::compressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString2);
    }

    // static int ccDeflateMemory(unsigned char*, unsigned int, unsigned char**);
    // static int ccDeflateMemoryWithHint(unsigned char*, unsigned int, unsigned char**, unsigned int);

    static gd::string compressString(gd::string const& data, bool encrypt, int key) {
        unsigned char* deflatedBuf = nullptr;
        auto deflatedSize = ccDeflateMemory((uint8_t*)(data.data()), data.size(), &deflatedBuf);

        if (deflatedSize < 1) {
            delete[] deflatedBuf;
            return data;
        }

        auto buffer = blaze::base64::encodeToString(deflatedBuf, deflatedSize, true);
        delete[] deflatedBuf;

        if (encrypt) {
            encryptDecryptImpl(buffer.data(), buffer.size(), key);
        }

        return buffer;
    }

    static gd::string decompressString(gd::string const& input, bool encrypted, int key) {
        if (input.empty()) {
            return "";
        }

        std::vector<uint8_t> rawData;
        if (!encrypted) {
            rawData = blaze::base64::decode(input, true);
        } else {
            auto decrypted = encryptDecrypt(input, key);
            rawData = blaze::base64::decode(decrypted, true);
        }

        if (rawData.empty()) {
            log::warn("decompressString fail 1");
            // if failed, try to fall back to original implementation, "just in case"
            return ZipUtils::decompressString(input, encrypted, key);
        }

        unsigned char* inflated = nullptr;
        size_t outlen = ccInflateMemory(rawData.data(), rawData.size(), &inflated);

        if (0 < outlen) {
            // success!
            return gd::string(reinterpret_cast<const char*>(inflated), outlen);
        }

        log::warn("decompressString fail 2");

        // if failed, try to fall back to original implementation, "just in case"
        return ZipUtils::decompressString(input, encrypted, key);
    }

    static gd::string decompressString2(unsigned char* data, bool encrypted, int size, int key) {
        if (!data || size < 1) {
            return ""; // TODO is it just "" or "\0" idk
        }


        if (encrypted) {
            // decrypt the data right into the original buffer
            encryptDecryptImpl(data, size, key);
        }

        auto sv = std::string_view((char*) data, size);

        std::vector<uint8_t> rawData = blaze::base64::decode(reinterpret_cast<char*>(data), size, true);
        if (rawData.empty()) {
            log::debug("decompressString2 fail 1");
            // if failed, try to fall back to original implementation
            if (encrypted) {
                encryptDecryptImpl(data, size, key);
            }

            return ZipUtils::decompressString2(data, encrypted, size, key);
        }

        unsigned char* inflated = nullptr;
        size_t outlen = ccInflateMemory(rawData.data(), rawData.size(), &inflated);

        if (0 < outlen) {
            // success!
            return gd::string(reinterpret_cast<const char*>(inflated), outlen);
        }

        log::warn("decompressString2 fail 2");

        // if failed, try to fall back to original implementation
        if (encrypted) {
            encryptDecryptImpl(data, size, key);
        }

        return ZipUtils::decompressString2(data, encrypted, size, key);
    }

    static gd::string encryptDecrypt(gd::string const& str, int key) {
        gd::string out{std::string_view(str)};
        encryptDecryptImpl(out.data(), out.size(), key);
        return out;
    }

    static void encryptDecryptImpl(void* data, size_t size, int key) {
        blaze::xor_u8(static_cast<uint8_t*>(data), size, static_cast<uint8_t>(key));
    }
};


