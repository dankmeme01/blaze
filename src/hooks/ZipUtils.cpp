#include <Geode/Geode.hpp>
#include <Geode/modify/ZipUtils.hpp>

#include <algo/base64.hpp>
#include <algo/compress.hpp>
#include <algo/xor.hpp>
#include <util.hpp>
#include <settings.hpp>
#include <TaskTimer.hpp>

using namespace geode::prelude;

// i thought of making it an option, but past level 1 it's really diminishing returns.
// level 0 is fastest but can be up to 50% bigger in size than level 1
// levels 1-12 have nearly identical decompression speed and size, but the compression speed grows up to multiple seconds.
static int compressionMode() {
    return blaze::settings().uncompressedSaves ? 0 : 1;
}

#define BLAZE_HOOK_COMPRESS 1
#define BLAZE_HOOK_DECOMPRESS 1

class $modify(ZipUtils) {
    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::compressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::decompressString2);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccInflateMemory);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccInflateMemoryWithHint);
        BLAZE_HOOK_VERY_LAST(cocos2d::ZipUtils::ccDeflateMemory);

        if (!blaze::settings().fastSaving) {
            if (auto h = self.getHook("cocos2d::ZipUtils::compressString")) {
                h.unwrap()->setAutoEnable(false);
            }

            if (auto h = self.getHook("cocos2d::ZipUtils::ccDeflateMemory")) {
                h.unwrap()->setAutoEnable(false);
            }
        }
    }

#if BLAZE_HOOK_DECOMPRESS
    static int ccInflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        blaze::Decompressor dec;
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
        auto result = decompressor.decompressToString(rawData.data(), rawData.size());

        if (result.isOk()) {
            // success!
#ifdef GEODE_IS_ANDROID
            return gd::string(std::move(result.unwrap()));
#else
            return std::move(result.unwrap());
#endif
        }

        log::warn("decompressString fail 2: {}", result.unwrapErr());

        // if failed, try to fall back to original implementation, "just in case"
        return ZipUtils::decompressString(input, encrypted, key);
    }

    static gd::string decompressString2(unsigned char* data, bool encrypted, int size, int key) {
        if (!data || size < 1) {
            return ""; // TODO is it just "" or "\0" idk
        }

        BLAZE_TIMER_START("decompressString2 (EncryptDecrypt)");

        if (encrypted) {
            // decrypt the data right into the original buffer
            encryptDecryptImpl(data, size, key);
        }

        BLAZE_TIMER_STEP("Base64");

        std::vector<uint8_t> rawData = blaze::base64::decode(reinterpret_cast<char*>(data), size, true);
        if (rawData.empty()) {
            log::debug("decompressString2 fail 1");
            // if failed, try to fall back to original implementation
            if (encrypted) {
                encryptDecryptImpl(data, size, key);
            }

            return ZipUtils::decompressString2(data, encrypted, size, key);
        }

        BLAZE_TIMER_STEP("Decompress");

        blaze::Decompressor decompressor;

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
#endif

#if BLAZE_HOOK_COMPRESS
    static int ccDeflateMemory(unsigned char* input, unsigned int size, unsigned char** outp) {
        blaze::Compressor compressor(compressionMode());
        compressor.setMode(blaze::CompressionMode::Gzip);

        auto chunk = compressor.compressToChunk(input, size);

        auto [outPtr, outSize] = chunk.release();

        *outp = outPtr;
        return outSize;
    }

    static gd::string compressString(gd::string const& data, bool encrypt, int key) {
        BLAZE_TIMER_START("compressString compression");

        blaze::Compressor compressor(compressionMode());
        compressor.setMode(blaze::CompressionMode::Gzip);

        auto compressedData = compressor.compress(data.data(), data.size());

        BLAZE_TIMER_STEP("base64");

        auto buffer = blaze::base64::encodeToString(compressedData.data(), compressedData.size(), true);

        BLAZE_TIMER_STEP("encryptDecrypt");

        if (encrypt) {
            encryptDecryptImpl(buffer.data(), buffer.size(), key);
        }

        BLAZE_TIMER_END();

        return buffer;
    }
#endif

    static void encryptDecryptImpl(void* data, size_t size, int key) {
        blaze::xor_u8(static_cast<uint8_t*>(data), size, static_cast<uint8_t>(key));
    }
};
