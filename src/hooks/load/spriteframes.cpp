#include "spriteframes.hpp"
#include <util/string.hpp>
#include <pugixml.hpp>

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

template <typename T>
T parseNode(pugi::xml_node node) {
    auto str = node.value();

    if constexpr (std::is_same_v<T, float>) {
        return blaze::parseFloat(str);
    } else if constexpr (std::is_same_v<T, int>) {
        return blaze::parseInt(str);
    } else if constexpr (std::is_same_v<T, bool>) {
        if (!str || !str[0]) {
            return false;
        }

        char c1 = str[0];
        char c2 = str[1];
        char c1m = c1 - '0';

        if (!c2 && (c1m == 1 || c1m == 0)) {
            return static_cast<bool>(c1m);
        }

        return strncmp(str, "false", 6) != 0;
    } else {
        static_assert(std::is_void_v<T>, "unsupported type");
    }
}

namespace blaze {

Result<std::unique_ptr<SpriteFrameData>> parseSpriteFrames(void* data, size_t size) {
    auto sfdata = std::make_unique<SpriteFrameData>();

    pugi::xml_document doc;
    auto result = doc.load_buffer_inplace(data, size);
    if (!result) {
        return Err("Failed to parse XML: {}", result.description());
    }

    pugi::xml_node plist = doc.child("plist");
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
            sfdata->metadata.format = parseNode<int>(valueNode);
            break; // Remove if we will need other properties
        }
    }

    if (sfdata->metadata.format < 0 || sfdata->metadata.format > 3) {
        return Err("Unsupported format version: {}", sfdata->metadata.format);
    }

    // Iterate over the frames
    for (pugi::xml_node keyNode = metadata.child("key"); keyNode; keyNode = keyNode.next_sibling("key")) {
        auto frameKey = keyNode.child_value();

        // the corresponding value node
        auto valueNode = keyNode.next_sibling();

        if (!valueNode) {
            continue;
        }
    }

    return Ok(std::move(sfdata));
}

void addSpriteFrames(const SpriteFrameData& dict, cocos2d::CCTexture2D* texture) {

}

} // namespace blaze
