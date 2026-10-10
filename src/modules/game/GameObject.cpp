#include "GameObject.hpp"
#include "hsv.hpp"

using namespace geode::prelude;

namespace blaze {

static thread_local std::optional<int> s_currentId;

static gd::string str(std::string_view s) {
    return gd::string{s.data(), s.size()};
}

std::pair<int, int> mapSpriteKeyAndSpecialId(int objectId) {
    int spriteKey = objectId;
    int specialId = -1;

    if (objectId == 0) return {0, -1};

    switch (objectId) {
        case 104: spriteKey = 915; specialId = 104; break;
        case 221: spriteKey = 899; specialId = 221; break;
        case 675: spriteKey = 1734; break;
        case 676: spriteKey = 1735; break;
        case 677: spriteKey = 1736; break;
        case 717: case 718: case 743:
            spriteKey = 899; specialId = objectId; break;
        case 1008: spriteKey = 1292; break;

        default: {
            if (objectId >= 1964 && objectId <= 2011) {
                spriteKey = 1964;
            } else {
                spriteKey = objectId;
            }
        }
    }

    return {spriteKey, specialId};
}

GameObject* BlazeGameObject::fromValues(ObjectProps& props, GJBaseGameLayer* layer, bool lowDetail) {
    bool highDetail = props.getBool(103);
    if (lowDetail && highDetail) return nullptr;

    // get object ID and map a sprite ID
    int objectID = props.getInt(1);
    auto [spriteKey, specialID] = mapSpriteKeyAndSpecialId(objectID);

    s_currentId = spriteKey;
    auto obj = static_cast<BlazeGameObject*>(GameObject::createWithKey(spriteKey));
    s_currentId = std::nullopt;
    if (!obj) return nullptr;

    gd::string frameName = ObjectToolbox::sharedState()->intKeyToFrame(spriteKey);

    // basic props

    // note: vanilla only does NaN checks, I added inf just in case
    float x = props.getFloatFinite(2);
    float y = props.getFloat(3) + 90.0f;
    if (!std::isfinite(y)) y = 0.0f;

    bool flipX = props.getBool(4);
    bool flipY = props.getBool(5);
    float rotation = props.getFloat(6);
    int editorLayer = props.getInt(20);
    int editorLayer2 = props.getInt(61);

    obj->m_objectID = spriteKey;
    if (spriteKey == 9 || spriteKey == 1715) {
        obj->m_defaultZOrder = 2;
    }

    obj->setCustomZLayer(props.getInt(24));
    obj->m_zOrder = props.getInt(25);
    obj->m_isHighDetail = highDetail;

    obj->addToGroup(props.getInt(26));
    obj->addToGroup(props.getInt(33));
    obj->m_linkedGroup = props.getInt(108);
    obj->m_isNoTouch = props.getBool(121);

    int enterChannel = std::clamp(props.getInt(343), 0, 100);
    obj->m_enterChannel = enterChannel;

    obj->m_objectMaterial = props.getInt(446);
    auto groupString = props.getString(57);
    obj->loadGroupsFromString(str(groupString));

    obj->m_hasGroupParent = props.getBool(34);
    obj->m_hasAreaParent = props.getBool(279);

    auto groupParentsString = props.getString(274);
    if (!groupParentsString.empty() && layer) {
        layer->loadGroupParentsFromString(obj, str(groupParentsString));
    } else {
        obj->m_hasGroupParentsString = false;
    }

    // scale
    float scaleX = props.getFloat(128);
    float scaleY = props.getFloat(129);
    if (scaleX != 0.0f) obj->updateCustomScaleX(scaleX);
    if (scaleY != 0.0f) obj->updateCustomScaleY(scaleY);

    // if neither x or y are set, use key 32 for both
    if (scaleX == 0.0f && scaleY == 0.0f) {
        float scale = props.getFloat(32);
        if (scale != 0.0f) {
            obj->updateCustomScaleX(scale);
            obj->updateCustomScaleY(scale);
        }
    }
    obj->m_startScaleX = obj->m_scaleX;
    obj->m_startScaleY = obj->m_scaleY;

    // misc flags
    auto loadFlag = [&]<typename T>(int key, T& field) {
        if constexpr (std::is_same_v<T, bool>) {
            field = props.getBool(key);
        } else {
            static_assert(std::is_same_v<T, int> || std::is_same_v<T, short>);
            field = static_cast<T>(props.getInt(key));
        }
    };

    loadFlag(64, obj->m_isDontFade);
    loadFlag(67, obj->m_isDontEnter);
    loadFlag(116, obj->m_hasNoEffects);
    loadFlag(507, obj->m_hasNoParticles);
    loadFlag(155, obj->m_mainColorKeyIndex);
    loadFlag(156, obj->m_detailColorKeyIndex);
    loadFlag(134, obj->m_isPassable);
    loadFlag(135, obj->m_isHide);
    loadFlag(136, obj->m_isNonStickX);
    loadFlag(289, obj->m_isNonStickY);
    loadFlag(495, obj->m_isExtraSticky);
    loadFlag(496, obj->m_isDontBoostY);
    loadFlag(509, obj->m_isDontBoostX);
    loadFlag(356, obj->m_isScaleStick);
    loadFlag(137, obj->m_isIceBlock);
    loadFlag(193, obj->m_isGripSlope);
    loadFlag(372, obj->m_hasNoAudioScale);
    loadFlag(497, obj->m_customColorType);
    loadFlag(511, obj->m_hasExtendedCollision);

    // polymorphic setup
    obj->customSetup();
    obj->customObjectSetup(props.m_values, props.m_present);
    obj->addGlow(frameName);
    obj->addColorSprite(frameName);
    obj->setupCustomSprites(frameName);

    // other flags
    obj->setFlipX(flipX);
    obj->setFlipY(flipY);
    obj->m_startFlipX = flipX;
    obj->m_startFlipY = flipY;
    obj->m_editorLayer = editorLayer;
    obj->m_editorLayer2 = editorLayer2;
    obj->m_hasNoGlow = props.getBool(96);

    // rotation snapping
    if (!obj->canRotateFree() && ((int)rotation % 90) != 0 && !obj->m_isNoTouch) {
        rotation = 0.0f;
    }

    obj->setRotation(rotation);
    obj->m_startRotationX = rotation;
    obj->m_startRotationY = rotation;

    // independent x/y rotation
    float rotX = props.getFloat(131);
    float rotY = props.getFloat(132);
    if (rotX != rotY && obj->canRotateFree()) {
        obj->setRotationX(rotX);
        obj->m_startRotationX = obj->getRotationX();
        obj->setRotationY(rotY);
        obj->m_startRotationY = obj->getRotationY();
    }

    if (spriteKey == 142) {
        reinterpret_cast<EnhancedGameObject*>(obj)->updateUserCoin();
    }

    obj->setStartPos({ x, y });
    obj->getObjectTextureRect(); // why did he call it here

    // special id handling
    switch (specialID) {
        case 104: reinterpret_cast<EffectGameObject*>(obj)->m_usesBlending = true; break;
        case 221: obj->m_targetColor = 1; break;
        case 717: obj->m_targetColor = 2; break;
        case 718: obj->m_targetColor = 3; break;
        case 743: obj->m_targetColor = 4; break;
        default: break;
    }

    // hsv
    if (props.getBool(41)) {
        auto hsv = parseHsv(str(props.getString(43)));
        if (hsv != DEFAULT_HSV) {          // vs. the default globals
            obj->m_baseColor->m_hsv = hsv;
            obj->m_baseColor->m_usesHSV = true;
        }
    }
    if (props.getBool(42) && obj->m_detailColor) {
        auto hsv = parseHsv(str(props.getString(44)));
        if (hsv != DEFAULT_HSV) {
            obj->m_detailColor->m_hsv = hsv;
            obj->m_detailColor->m_usesHSV = true;
        }
    }

    // color channels
    if (int presetColor = props.getInt(19)) {
        std::array<int, 8> channelIdTable = { 1005, 1006, 1, 2, 1007, 3, 4, 1003 };

        size_t idx = presetColor - 1;
        if (idx < channelIdTable.size()) {
            int colorID = channelIdTable[idx];
            auto* target = obj->m_detailColor ? obj->m_detailColor : obj->m_baseColor;
            target->m_colorID = colorID;
        }
    } else {
        int baseChannel = std::clamp(props.getInt(21), 0, 1101);
        if (baseChannel != 0) {
            obj->m_baseColor->m_colorID = baseChannel;
        }

        if (obj->m_detailColor) {
            if (int detailChannel = std::clamp(props.getInt(22), 0, 1101)) {
                obj->m_detailColor->m_colorID = detailChannel;
            }
        }
    }

    obj->saveActiveColors();
    obj->resetRScaleForced();

    return obj;
}

GameObject* BlazeGameObject::createWithFrame(char const* name) {
    auto id = s_currentId;
    if (!id) return GameObject::createWithFrame(name);

    // cool
    auto sf = GameModule::get().spriteFrameForObjectId(*id);
    if (!sf) return GameObject::createWithFrame(name);

    auto obj = new GameObject();

    // init is inlined so
    if (obj->initWithSpriteFrame(sf)) {
        obj->commonSetup();
        obj->m_bUnkBool2 = true;
        obj->autorelease();
        return obj;
    }

    delete obj;
    return nullptr;

}

void BlazeGameObject::loadGroupsFromString(gd::string groupList) {
    if (groupList.empty()) return;

    auto sv = std::string_view{groupList.data(), groupList.size()};

    if (!m_groups) {
        int cap = 10;
        if (!m_editorEnabled) {
            cap = asp::iter::from(sv).filter([](char c) { return c == '.'; }).count() + 1;
        }
        this->createGroupContainer(cap);
    }

    asp::iter::split(sv, '.')
        .map([](auto s) {
            return utils::numFromString<short>(s).unwrapOr(0);
        })
        .forEach([&](short s) {
            (*m_groups)[m_groupCount++] = s;
        });
}

void BlazeGameObject::createGroupContainer(int size) {
    if (m_groups) return;
    m_groups = (std::array<short, 10>*)new short[size]{};
}

}