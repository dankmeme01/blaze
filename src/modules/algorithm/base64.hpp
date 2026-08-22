#pragma once
#include <Geode/utils/base64.hpp>

namespace blaze::base64 {

using geode::utils::base64::Base64Variant;

inline std::string encode(std::span<const uint8_t> data, bool url = true) {
    auto encoded = geode::utils::base64::encode(data, url ? Base64Variant::Url : Base64Variant::Normal);

    // cocos peculiarities
    encoded.push_back('=');
    encoded.push_back('\0');

    return encoded;
}

template <bool String = false>
inline auto decode(std::string_view data, bool url = true) {
    // a weird peculiarity of gd/cocos is that data can have some trailing garbage at the end, we want to avoid decompressing that

    // grab last 8 characters
    auto ending = data.substr(data.size() - std::min<size_t>(data.size(), 8));

    for (auto [i, c] : asp::iter::enumerate(ending)) {
        if (c == '=' || c == '\0') {
            size_t trailingGarbage = ending.size() - i;
            data.remove_suffix(trailingGarbage);
            break;
        }
    }

    // TODO we could just use simdutf directly to avoid costly things geode does, but for now it's ok
    auto mode = url ? Base64Variant::Url : Base64Variant::NormalNoPad;
    if constexpr (String) {
        return geode::utils::base64::decodeString(data, mode);
    } else {
        return geode::utils::base64::decode(data, mode);
    }
}

}
