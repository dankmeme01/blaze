#include <Geode/Geode.hpp>
#include <Geode/modify/ZipUtils.hpp>

#include <Geode/loader/SettingEvent.hpp>

#include <algo/base64.hpp>
#include <algo/compress.hpp>
#include <algo/xor.hpp>
#include <util.hpp>
#include <TaskTimer.hpp>

using namespace geode::prelude;

// i thought of making it an option, but past level 1 it's really diminishing returns.
// level 0 is fastest but can be up to 50% bigger in size than level 1
// levels 1-12 have nearly identical decompression speed and size, but the compression speed grows up to multiple seconds.

static int COMPRESSION_MODE = 1;

$on_mod(Loaded) {
    bool store = Mod::get()->getSettingValue<bool>("uncompressed-saves");

    COMPRESSION_MODE = store ? 0 : 1;

    listenForSettingChanges("uncompressed-saves", +[](bool value) {
        COMPRESSION_MODE = value ? 0 : 1;
    });
}

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
        blaze::Compressor compressor(COMPRESSION_MODE);

        // Instead of gzip, we use zlib for compression.
        // I honestly have no idea why this is needed, but I believe it's due to a difference between libdeflate and zlib.
        // GD uses zlib in a gzip-compatible format, whereas blaze uses libdeflate with gzip mode for decompression and zlib mode for compression.
        // Here's the results of my testing:
        //
        // save with zlib in gzip mode -> load with libdeflate in gzip mode        - success.
        // save with zlib in gzip mode -> load with libdeflate in zlib mode        - invalid data error.
        // save with libdeflate in gzip mode -> load with libdeflate in gzip mode  - success.
        // save with libdeflate in gzip mode -> load with zlib in gzip mode        - load error*.
        // save with libdeflate in zlib mode -> load with libdeflate in gzip mode  - success.
        // save with libdeflate in zlib mode -> load with zlib in gzip mode        - success.
        //
        // This weird behavior essentially leaves us with one option if we want our saves to be vanilla-compatible,
        // which is to save in zlib mode and load in gzip mode.
        compressor.setMode(blaze::CompressionMode::Zlib);

        auto chunk = compressor.compressToChunk(input, size);

        auto [outPtr, outSize] = chunk.release();

        *outp = outPtr;
        return outSize;
    }

    static gd::string compressString(gd::string const& data, bool encrypt, int key) {
        BLAZE_TIMER_START("compressString compression");

        blaze::Compressor compressor(COMPRESSION_MODE);
        compressor.setMode(blaze::CompressionMode::Zlib);

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

        BLAZE_TIMER_START("EncryptDecrypt");

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

    static gd::string encryptDecrypt(gd::string const& str, int key) {
        gd::string out{std::string_view(str)};
        encryptDecryptImpl(out.data(), out.size(), key);
        return out;
    }

    static void encryptDecryptImpl(void* data, size_t size, int key) {
        blaze::xor_u8(static_cast<uint8_t*>(data), size, static_cast<uint8_t>(key));
    }
};
