#include <Geode/Geode.hpp>
#include <Geode/modify/ZipUtils.hpp>
#include <Geode/cocos/support/base64.h>

#include <algo/base64.hpp>
#include <util.hpp>

using namespace geode::prelude;

static int base64DecodeHook(unsigned char *in, unsigned int inLength, unsigned char **out) {
    size_t outLen = blaze::base64::decodedLen(reinterpret_cast<char*>(in), inLength);

    auto buf = new uint8_t[outLen];
    outLen = blaze::base64::decode(reinterpret_cast<char*>(in), inLength, buf, true);
    if (outLen == 0) {
        log::warn("base64 decode failed");
        delete[] buf;
        buf = nullptr;
    }

    *out = buf;

    return outLen;
}

$execute {
    auto hook = Mod::get()->hook(
        reinterpret_cast<void*>(addresser::getNonVirtual(cocos2d::base64Decode)),
        base64DecodeHook,
        "cocos2d::base64Decode"
    ).unwrap();

    hook->setPriority(1999999999); // very last

    // benchmark code
    // auto genrandom = reinterpret_cast<BOOLEAN (*)(PVOID, ULONG)>(GetProcAddress(GetModuleHandle("Advapi32.dll"), "SystemFunction036"));

    // size_t bufSize = 1024 * 1024 * 512;
    // auto buf = new uint8_t[bufSize];
    // genrandom(buf, bufSize);

    // size_t outs = blaze::base64::encodedLen(bufSize);

    // auto outbuf = new char[outs];

    // auto preEncode = benchTimer();
    // size_t encSize = blaze::base64::encode(buf, bufSize, outbuf);
    // log::debug("Predicted size: {}, enc size: {}, time to encode: {}", outs, encSize, formatDuration(benchTimer() - preEncode));
    // log::debug("size to decode into: {}, should be: {}", blaze::base64::decodedLen(outbuf, encSize), bufSize);

    // auto preDecode = benchTimer();
    // size_t decSize = blaze::base64::decode(outbuf, encSize, buf);
    // log::debug("Decoded size: {}, time to decode: {}", decSize, formatDuration(benchTimer() - preDecode));

    // delete[] buf;
    // delete[] outbuf;

    // std::exit(0);
}

