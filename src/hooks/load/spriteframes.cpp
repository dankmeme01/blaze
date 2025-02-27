#include "spriteframes.hpp"
#include <util/string.hpp>
#include <util/hash.hpp>
#include <util.hpp>

#include <pugixml.hpp>
#include <fast_float/fast_float.h>

#include <Geode/loader/Log.hpp>
#include <Geode/Result.hpp>
#include <Geode/Prelude.hpp>
#include <Geode/utils/general.hpp>
#include <fmt/core.h>

using namespace geode::prelude;

// big hack to call a private cocos function
namespace {
    template <typename TC>
    using priv_method_t = void(TC::*)(CCDictionary*, CCTexture2D*);

    template <typename TC, priv_method_t<TC> func>
    struct priv_caller {
        friend void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2) {
            auto* obj = CCSpriteFrameCache::sharedSpriteFrameCache();
            (obj->*func)(p1, p2);
        }
    };

    template struct priv_caller<CCSpriteFrameCache, &CCSpriteFrameCache::addSpriteFramesWithDictionary>;

    void _addSpriteFramesWithDictionary(CCDictionary* p1, CCTexture2D* p2);
}

namespace blaze {

template <typename T = CCPoint>
std::optional<T> parseCCPoint(std::string_view str) {
    // A point is formatted in form {x,y}.
    // Cocos does a bunch of unnecessary checks here, we are just going to go by the following rules:
    // * First character has to be an opening brace
    // * First number is parsed after the first brace
    // * At the end of the first number, a comma must be present
    // * Second number is parsed after the comma
    // * At the end of the second number, a closing brace must be present
    float x = 0.f, y = 0.f;

    if (str.size() < 5) {
        return std::nullopt;
    }

    if (str[0] != '{') {
        return std::nullopt;
    }

    auto result = fast_float::from_chars(&str[1], &*str.end(), x);
    if (result.ec != std::errc{}) {
        return std::nullopt;
    }

    auto postFirstNumber = result.ptr;
    if (*postFirstNumber != ',') {
        return std::nullopt;
    }

    auto postComma = postFirstNumber + 1;
    result = fast_float::from_chars(postComma, &*str.end(), y);

    if (result.ec != std::errc{}) {
        return std::nullopt;
    }

    auto postSecondNumber = result.ptr;
    if (*postSecondNumber != '}') {
        return std::nullopt;
    }

    return T{x, y};
}

std::optional<CCRect> parseCCRect(std::string_view str) {
    // A rect is formatted as {{x,y},{w,h}}.
    // We are going to go by the following rules:
    // * First character has to be an opening brace, and last character has to be a closing brace
    // * We find the index of the second comma, and split the string into two substrings (omitting the very first and very last braces)
    // * Call parseCCPoint on both substrings and use their results

    CCPoint origin;
    CCSize size;

    if (str[0] != '{' || str[str.size() - 1] != '}') {
        return std::nullopt;
    }

    size_t secondCommaIdx = 0;
    bool isFirstComma = true;
    for (size_t i = 0; i < str.size(); i++) {
        char c = str[i];
        if (c == ',') {
            if (isFirstComma) {
                isFirstComma = false;
            } else {
                secondCommaIdx = i;
                break;
            }
        }
    }

    if (secondCommaIdx == 0) {
        return std::nullopt;
    }

    auto originStr = str.substr(1, secondCommaIdx - 1);
    auto sizeStr = str.substr(secondCommaIdx + 1, str.size() - secondCommaIdx - 2);

    if (auto x = parseCCPoint(originStr)) {
        origin = x.value();
    } else {
        return std::nullopt;
    }

    if (auto x = parseCCPoint<CCSize>(sizeStr)) {
        size = x.value();
    } else {
        return std::nullopt;
    }

    return CCRect{origin, size};
}

template <typename T>
std::optional<T> parseNode(pugi::xml_node node) {
    auto str = node.child_value();

    if constexpr (std::is_same_v<T, float>) {
        return blaze::parseFloat(str);
    } else if constexpr (std::is_same_v<T, int>) {
        return blaze::parseInt(str);
    } else if constexpr (std::is_same_v<T, bool>) {
        return strncmp(node.name(), "true", 5) == 0;
    } else if constexpr (std::is_same_v<T, CCPoint> || std::is_same_v<T, CCSize>) {
        return parseCCPoint<T>(str);
    } else if constexpr(std::is_same_v<T, CCRect>) {
        return parseCCRect(str);
    } else {
        static_assert(std::is_void_v<T>, "unsupported type");
    }
}

#ifdef BLAZE_DEBUG
# define assign_or_bail(var, val) \
    if (auto _res = (val)) { \
        var = std::move(_res).value(); \
    } else { \
        log::debug("Failed to parse {} ({}:{})", #var, __FILE__, __LINE__); \
        return false; \
    } \

#else
# define assign_or_bail(var, val) if (auto _res = (val)) { var = std::move(_res).value(); } else { return false; }
#endif

bool parseSpriteFrameV0(pugi::xml_node node, SpriteFrame& sframe) {
    sframe.textureRotated = false;

    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = blaze::hashStringRuntime(keyName);
        switch (keyHash) {
            case BLAZE_STRING_HASH("x"): {
                assign_or_bail(sframe.textureRect.origin.x, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("y"): {
                assign_or_bail(sframe.textureRect.origin.y, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("width"): {
                assign_or_bail(sframe.textureRect.size.width, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("height"): {
                assign_or_bail(sframe.textureRect.size.height, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("offsetX"): {
                assign_or_bail(sframe.offset.x, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("offsetY"): {
                assign_or_bail(sframe.offset.y, parseNode<float>(valueNode));
            } break;

            case BLAZE_STRING_HASH("originalWidth"): {
                assign_or_bail(sframe.sourceSize.width, (parseNode<int>(valueNode)));
                sframe.sourceSize.width = std::abs(sframe.sourceSize.width);
            } break;

            case BLAZE_STRING_HASH("originalHeight"): {
                assign_or_bail(sframe.sourceSize.height, (parseNode<int>(valueNode)));
                sframe.sourceSize.height = std::abs(sframe.sourceSize.height);
            } break;
        }
    }

    return true;
}

bool parseSpriteFrameV1_2(pugi::xml_node node, SpriteFrame& sframe, int format) {
    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = blaze::hashStringRuntime(keyName);
        switch (keyHash) {
            case BLAZE_STRING_HASH("frame"): {
                assign_or_bail(sframe.textureRect, parseNode<cocos2d::CCRect>(valueNode));
            } break;

            case BLAZE_STRING_HASH("rotated"): {
                if (format == 2) {
                    assign_or_bail(sframe.textureRotated, parseNode<bool>(valueNode));
                }
            } break;

            case BLAZE_STRING_HASH("offset"): {
                assign_or_bail(sframe.offset, parseNode<cocos2d::CCPoint>(valueNode));
            } break;

            case BLAZE_STRING_HASH("sourceSize"): {
                assign_or_bail(sframe.sourceSize, parseNode<cocos2d::CCSize>(valueNode));
            } break;
        }
    }

    return true;
}

bool parseSpriteFrameV3(pugi::xml_node node, SpriteFrame& sframe) {
    for (pugi::xml_node keyNode = node.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        auto keyHash = blaze::hashStringRuntime(keyName);

        switch (keyHash) {
            case BLAZE_STRING_HASH("spriteOffset"): {
                assign_or_bail(sframe.offset, parseNode<cocos2d::CCPoint>(valueNode));
            } break;

            // Apparently unused?
            // case BLAZE_STRING_HASH("spriteSize"): {
            //     sframe.size = parseNode<cocos2d::CCSize>(valueNode);
            // } break;

            case BLAZE_STRING_HASH("spriteSourceSize"): {
                assign_or_bail(sframe.sourceSize, parseNode<cocos2d::CCSize>(valueNode));
            } break;

            case BLAZE_STRING_HASH("textureRect"): {
                assign_or_bail(sframe.textureRect, parseNode<cocos2d::CCRect>(valueNode));
            } break;

            case BLAZE_STRING_HASH("textureRotated"): {
                assign_or_bail(sframe.textureRotated, parseNode<bool>(valueNode));
            } break;

            case BLAZE_STRING_HASH("aliases"): {
                // It seems like aliases are never used by GD, so I cannot test this code, but in theory it should work..
                for (pugi::xml_node aliasNode = valueNode.child("string"); aliasNode; aliasNode = aliasNode.next_sibling("string")) {
                    sframe.aliases.push_back(aliasNode.child_value());
                }
            } break;
        }
    }

    return true;
}

bool parseSpriteFrame(pugi::xml_node node, SpriteFrame& sframe, int format) {
    switch (format) {
        case 0: return parseSpriteFrameV0(node, sframe);
        case 1: [[fallthrough]];
        case 2: return parseSpriteFrameV1_2(node, sframe, format);
        case 3: return parseSpriteFrameV3(node, sframe);
        default: blaze::unreachable(); // this is already handled by parseSpriteFrames
    }
}

Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size) {
    auto sfdata = std::make_unique<SpriteFrameData>();

    auto result = sfdata->doc.load_buffer_inplace(data, size);
    if (!result) {
        return Err("Failed to parse XML: {}", result.description());
    }

    pugi::xml_node plist = sfdata->doc.child("plist");
    if (!plist) {
        return Err("Failed to find root <plist> node");
    }

    pugi::xml_node rootDict = plist.child("dict");
    if (!rootDict) {
        return Err("Failed to find root <dict> node");
    }

    pugi::xml_node frames, metadata;

    for (pugi::xml_node keyNode = rootDict.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        if (strcmp(keyName, "frames") == 0) {
            frames = valueNode;
        } else if (strcmp(keyName, "metadata") == 0) {
            metadata = valueNode;
        }
    }

    if (!frames) {
        return Err("Failed to find 'frames' node");
    }

    if (!metadata) {
        return Err("Failed to find 'metadata' node");
    }

    // Iterate over the metadata, although we only need the format version
    for (pugi::xml_node keyNode = metadata.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto keyName = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }

        if (strcmp(keyName, "format") == 0) {
            sfdata->metadata.format = parseNode<int>(valueNode).value_or(-1);

#ifndef BLAZE_DEBUG
            break; // we only need the format version, break early if not in debug mode
#endif
        }

        if (strcmp(keyName, "realTextureFileName") == 0) {
            sfdata->metadata.textureFileName = valueNode.child_value();
        }
    }

    if (sfdata->metadata.format < 0 || sfdata->metadata.format > 3) {
        return Err("Unsupported format version: {}", sfdata->metadata.format);
    }

    // Note: one could try and optimize this by counting the amount of children and reserving space in the `sfData->frames`.
    // In my tests this proved to be ever so slightly slower, although it could depend on the platform and device.
    // Therefore this optimization was not done here.

    // Iterate over the frames
    for (pugi::xml_node keyNode = frames.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto frameKey = keyNode.child_value();

        // the corresponding value node
        auto frameDict = keyNode.next_sibling();

        if (!frameDict) {
            continue;
        }

        SpriteFrame frame;
        frame.name = frameKey;

        if (!parseSpriteFrame(frameDict, frame, sfdata->metadata.format)) {
            log::warn("Failed to parse frame '{}', skipping!", frameKey);
            continue;
        }

        sfdata->frames.push_back(std::move(frame));
    }

    return Ok(std::move(sfdata));
}

void addSpriteFrames(const SpriteFrameData& frames, cocos2d::CCTexture2D* texture) {
    auto sfcache = CCSpriteFrameCache::get();

    for (const auto& frame : frames.frames) {
        // create sprite frame
        auto spriteFrame = new CCSpriteFrame();
        bool result = spriteFrame->initWithTexture(
            texture,
            frame.textureRect,
            frame.textureRotated,
            frame.offset,
            frame.sourceSize
        );

        if (!result) {
            log::warn("Failed to initialize sprite frame for {}", frame.name);
            continue;
        }

        // add sprite frame
        sfcache->m_pSpriteFrames->setObject(spriteFrame, frame.name);

        // if there are any aliases, add them as well
        if (!frame.aliases.empty()) {
            // create one CCString and reuse it
            auto fnamestr = CCString::create(frame.name);

            for (const auto& alias : frame.aliases) {
                sfcache->m_pSpriteFramesAliases->setObject(
                    fnamestr,
                    alias
                );
            }
        }

        spriteFrame->release();
    }
}

void addSpriteFramesVanilla(cocos2d::CCDictionary* dict, cocos2d::CCTexture2D* texture) {
    _addSpriteFramesWithDictionary(dict, texture);
}

} // namespace blaze
