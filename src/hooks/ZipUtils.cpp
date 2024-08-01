#include <Geode/Geode.hpp>
#include <Geode/modify/ZipUtils.hpp>

#include <algo/base64.hpp>
#include <algo/compress.hpp>
#include <algo/xor.hpp>
#include <util.hpp>

using namespace geode::prelude;

constexpr static int COMPRESSION_MODE = 3;

class $modify(ZipUtils) {
    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::compressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString2);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccInflateMemory);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccInflateMemoryWithHint);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccDeflateMemory);
    }

    static int ccInflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        blaze::Decompressor dec;
        dec.setMode(blaze::CompressionMode::Gzip);
        auto chunk = dec.decompressToChunk(input, size);

        if (!chunk) {
            log::warn("ccDeflateMemory failed, calling original: {}", chunk.unwrapErr());
            return ZipUtils::ccDeflateMemory(input, size, outp);
        }

        auto [outPtr, outSize] = chunk->release();

        *outp = outPtr;
        return outSize;
    }

    static int ccInflateMemoryWithHint(unsigned char* input, unsigned int size, unsigned char** outp, unsigned int hint) {
        return ccInflateMemory(input, size, outp);
    }

    static int ccDeflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        blaze::Compressor dec(COMPRESSION_MODE);
        dec.setMode(blaze::CompressionMode::Gzip);

        auto chunk = dec.compressToChunk(input, size);

        auto [outPtr, outSize] = chunk.release();

        *outp = outPtr;
        return outSize;
    }

    static gd::string compressString(gd::string const& data, bool encrypt, int key) {
        blaze::Compressor compressor(COMPRESSION_MODE);
        compressor.setMode(blaze::CompressionMode::Gzip);

        auto compressedData = compressor.compress(data.data(), data.size());

        auto buffer = blaze::base64::encodeToString(compressedData.data(), compressedData.size(), true);

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

        blaze::Decompressor decompressor;
        decompressor.setMode(blaze::CompressionMode::Gzip);
        auto result = decompressor.decompressToString(rawData.data(), rawData.size());

        if (result.isOk()) {
            // success!
#ifdef GEODE_IS_ANDROID
            return gd::string(std::move(result.unwrap()));
#else
            return std::move(result.unwrap());
#endif
        }

        log::warn("decompressString2 fail 2: {}", result.unwrapErr());

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

        blaze::Decompressor decompressor;
        decompressor.setMode(blaze::CompressionMode::Gzip);

        auto result = decompressor.decompressToString(rawData.data(), rawData.size());

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

    static gd::string encryptDecrypt(gd::string const& str, int key) {
        gd::string out{std::string_view(str)};
        encryptDecryptImpl(out.data(), out.size(), key);
        return out;
    }

    static void encryptDecryptImpl(void* data, size_t size, int key) {
        blaze::xor_u8(static_cast<uint8_t*>(data), size, static_cast<uint8_t>(key));
    }
};
