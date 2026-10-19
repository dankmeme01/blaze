#include "GameObjectBuilder.hpp"
#include "ObjectLoader.hpp"
#include "generated.hpp"
#include "objects.hpp"
#include <modules/engine/CCSprite.hpp>
#include <modules/game/GameModule.hpp>
#include <modules/game/hsv.hpp>

using namespace geode::prelude;

namespace blaze {

static std::mutex g_mutex;

using State = GameObjectBuilder::State;

static int fastParseKey(std::string_view sv) {
    int val = 0;
    for (char c : sv) {
        if (c < '0' || c > '9') [[unlikely]] {
            return 0;
        }
        val = val * 10 + (c - '0');
    }
    return val;
}

static std::string str(std::string_view s) {
    return std::string{s.data(), s.size()};
}

static bool isSafeForMultithread(int id) {
    switch (id) {
        // start pos, does fancy stuff
        case 31: return false;

        // text, can be done later
        case 914: return false;

        // animated game object, doable but annoying
        case 918 ... 919:
        case 1327 ... 1328:
        case 1584:
        case 2012: return false;

        default: {
            return id < 100000000;
        }
    }
}

static std::pair<int, int> mapSpriteKeyAndSpecialId(int objectId) {
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

GameObjectBuilder::GameObjectBuilder(std::string* source, uint32_t index)
    : m_source(source), m_index(index), m_state{State::Initial} {}

GameObjectBuilder::GameObjectBuilder(GameObjectBuilder&& other) noexcept {
    *this = std::move(other);
}

GameObjectBuilder& GameObjectBuilder::operator=(GameObjectBuilder&& other) noexcept {
    if (this != &other) {
        this->cleanup();
        m_object = other.m_object;
        m_source = other.m_source;
        m_props = std::move(other.m_props);
        m_index = other.m_index;
        m_state.store(other.m_state.load(std::memory_order::acquire), std::memory_order::release);
        other.m_object = nullptr;
        other.m_source = nullptr;
        other.m_state = State::Initial;
    }
    return *this;
}

GameObjectBuilder::~GameObjectBuilder() {
    this->cleanup();
}

void GameObjectBuilder::cleanup() {
    if (m_source) m_source->clear();

    m_props.reset();
}

GameObject* GameObjectBuilder::takeOutObject() {
    auto obj = m_object.take();
    if (!obj) return nullptr;

    obj->autorelease();
    return obj;
}

bool GameObjectBuilder::needsOrderedPolyInit() {
    return true;
    // return m_needsPolyInitOrder;
}

GameObjectAdvanceResult GameObjectBuilder::advance(ObjectsLoadState& state, bool mainThread) {
    switch (this->state()) {
        case State::Initial: {
            this->parseProps(state);
            return GameObjectAdvanceResult::Pending;
        } break;

        case State::Parsed: {
            int objectId = m_props.getInt(1);
            if (!isSafeForMultithread(objectId)) {
                if (mainThread) {
                    // run GameObject::createWithKey on main thread
                    this->createObjectWithKey(objectId);
                } else {
                    // not main thread and a non-thread-safe object, require main thread
                    return GameObjectAdvanceResult::RequiresMainThread;
                }
            } else {
                this->createObjectCustom(objectId);
            }

            this->setState(m_object ? State::Created : State::Skipped);

            return GameObjectAdvanceResult::Pending;
        } break;

        case State::Created: {
            this->loadBasicProperties(state);
            return GameObjectAdvanceResult::RequiresMainThread;
        } break;

        case State::BasicDone:
        case State::PolyInitMainPending: {
            this->doPolymorphicInit(state);
            return GameObjectAdvanceResult::Pending;
        } break;

        case State::PolyInitDone: {
            this->loadRemainingProperties(state);
            return GameObjectAdvanceResult::RequiresMainThread;
        } break;

        case State::AllPropsDone: {
            B_DEBUG_ASSERT(mainThread);
            this->performPostInit(state);
            this->performFinalCleanup();
            return GameObjectAdvanceResult::Finished;
        } break;

        case State::Skipped:
        case State::Finished: {
            return GameObjectAdvanceResult::Finished;
        } break;
    }

    std::unreachable();
}

void GameObjectBuilder::parseProps(ObjectsLoadState& state) {
    B_ASSERT(m_source && "source string not set");

    // string in format k,v,k,v
    for (auto [k, v] : asp::iter::split(*m_source, ',').arrayChunks<2>()) {
        int key = fastParseKey(k);

        if (key >= 1 && key <= 600) {
            m_props.set(key, v);

            if (key == 103 && state.m_ldm && m_props.getBool(103)) {
                // skip this object entirely since we are ldm
                this->setState(State::Skipped);
                return;
            }
        }
    }

    std::tie(m_spriteKey, std::ignore) = mapSpriteKeyAndSpecialId(m_props.getInt(1));

    this->setState(State::Parsed);
}

void GameObjectBuilder::createObjectWithKey(int objectId) {
    auto obj = GameObject::createWithKey(m_spriteKey);

    m_object = obj;
    m_autoreleased = true;
}

void GameObjectBuilder::createObjectCustom(int objectId) {
    auto frame = GameModule::get().spriteFrameForObjectId(m_spriteKey);
    m_object = createGameObject(m_spriteKey, frame, &m_needsCustomSetup);
    // m_needsPolyInitOrder = m_needsCustomSetup;

    if (m_object->m_unk390 == 7) {
        m_object->m_unk390 = static_cast<int>(m_object->m_objectType);
    }

    m_autoreleased = false;
}

void GameObjectBuilder::loadBasicProperties(ObjectsLoadState& state) {
    auto& props = m_props;
    auto obj = m_object.data();

    // make sure that the unique ID is sequential in regards to the order in level string, actual value does not matter much
    // multiply by 16 because uhh umm each object may create additional helper objects in `addObject` and they will try to draw some other id, keep space of 15 slots after this idx
    auto uniqueId = state.m_uniqueIdBase + m_index * 16;
    obj->m_uniqueID = uniqueId;
    obj->m_unknown5 = uniqueId;

    obj->m_objectID = m_spriteKey;
    if (m_spriteKey == 9 || m_spriteKey == 1715) {
        obj->m_defaultZOrder = 2;
    }

    obj->setCustomZLayer(props.getInt(24));
    obj->m_zOrder = props.getInt(25);
    obj->m_isHighDetail = props.getBool(103);

    obj->addToGroup(props.getInt(26));
    obj->addToGroup(props.getInt(33));
    obj->m_linkedGroup = props.getInt(108);
    obj->m_isNoTouch = props.getBool(121);

    int enterChannel = std::clamp(props.getInt(343), 0, 100);
    obj->m_enterChannel = enterChannel;

    obj->m_objectMaterial = props.getInt(446);
    auto groupString = props.getString(57);
    this->loadGroupsFromString(str(groupString));

    obj->m_hasGroupParent = props.getBool(34);
    obj->m_hasAreaParent = props.getBool(279);

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

    this->setState(State::BasicDone);
}

static bool isSafeToSetupCustomSprites(int objectId) {
    std::array unsafe = {
        85, 154, 180, 183, 186, 222, 289, 394, 397,
        675, 678, 740,
        939,
        1055, 1056, 1057,
        1338, 1339,
        1341, 1342, 1344, 1345,
        1743, 1744, 1745, 1746, 1747, 1748, 1749, 1750, // slope outline, not easily hookable
    };

    return !std::ranges::binary_search(unsafe, objectId);
}

void GameObjectBuilder::doPolymorphicInit(ObjectsLoadState& state) {
    auto obj = m_object.data();

    auto groupParentsString = m_props.getString(274);
    if (!groupParentsString.empty() && state.m_gameLayer) {
        state.m_gameLayer->loadGroupParentsFromString(obj, str(groupParentsString));
    } else {
        obj->m_hasGroupParentsString = false;
    }

    obj->customSetup();

    if (m_needsCustomSetup) {
        // we don't need to call this for plain GameObjects, they do not override it
        auto scratch = state.getScratch(m_props);
        obj->customObjectSetup(scratch.m_values, scratch.m_present);
        state.returnScratch(std::move(scratch));
    }

    if (!isSafeToSetupCustomSprites(m_spriteKey)) {
        // must do all this on main thread
        auto frameName = GameModule::get().frameNameForObjectId(m_spriteKey);
        this->addColorSprite(frameName);
        this->addGlow(frameName);
        obj->setupCustomSprites(frameName);
        m_needsSetupSprites = false;
    }

    this->setState(State::PolyInitDone);
}

void GameObjectBuilder::loadRemainingProperties(ObjectsLoadState& state) {
    auto& props = m_props;
    auto obj = m_object.data();

    // take this opportunity to call some thread-safe reimplementations, if possible
    if (m_needsSetupSprites) {
        auto frameName = GameModule::get().frameNameForObjectId(m_spriteKey);
        this->addColorSprite(frameName);
        this->addGlow(frameName);
        // hook in GameObject.cpp makes this thread safe-ish for most objects
        log::debug("I am {}", m_spriteKey);
        obj->setupCustomSprites(frameName);
        m_needsSetupSprites = false;
    }

    bool flipX = props.getBool(4);
    bool flipY = props.getBool(5);
    float rotation = props.getFloat(6);
    int editorLayer = props.getInt(20);
    int editorLayer2 = props.getInt(61);

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

    // note: vanilla only does NaN checks, I added inf just in case
    float x = props.getFloatFinite(2);
    float y = props.getFloat(3) + 90.0f;
    if (!std::isfinite(y)) y = 0.0f;

    obj->setStartPos({ x, y });
    obj->getObjectTextureRect(); // why did he call it here

    // special id handling
    auto [_, specialID] = mapSpriteKeyAndSpecialId(m_props.getInt(1));

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

    obj->resetRScaleForced();
    // obj->saveActiveColors() happens on main thread later

    // clear the memory right away
    m_props = ObjectProps<true>{};

    this->setState(State::AllPropsDone);
}

void GameObjectBuilder::performPostInit(ObjectsLoadState& state) {
    auto obj = m_object.data();
    auto pl = state.m_gameLayer;

    // gold coins can only appear in main levels
    if (obj->getType() == GameObjectType::SecretCoin && state.m_levelType != GJLevelType::Main) {
        this->setState(State::Skipped);
        return;
    }

    // hard coded rob stuff
    if (obj->m_objectID == 31) {
        static_cast<StartPosObject*>(obj)->loadSettingsFromString(*m_source);
    } else if (obj->m_objectID == 2065) {
        static_cast<ParticleGameObject*>(obj)->updateParticleStruct();
    }

    this->setState(State::Finished);
}

// faster rewrite of loadGroupsFromString
void GameObjectBuilder::loadGroupsFromString(std::string_view sv) {
    if (sv.empty()) return;

    if (!m_object->m_groups) {
        int cap = 10;
        if (!m_object->m_editorEnabled) {
            cap = asp::iter::from(sv).filter([](char c) { return c == '.'; }).count() + 1;
        }

        m_object->m_groups = (std::array<short, 10>*)new short[cap]{};
    }

    asp::iter::split(sv, '.')
        .map([](auto s) {
            return utils::numFromString<short>(s).unwrapOr(0);
        })
        .forEach([&](short s) {
            (*m_object->m_groups)[m_object->m_groupCount++] = s;
        });
}

static CCSpriteFrame* findFrame(const std::string& key) {
    return (CCSpriteFrame*)CCSpriteFrameCache::get()->m_pSpriteFrames->objectForKey(key);
}

void GameObjectBuilder::addColorSprite(std::string_view sv) {
    uint32_t index = m_spriteKey - 207;
    if (index >= sizeof(COLOR_SPRITE_MAP)) return;

    int defaultChannel = -1; // -1: keep the GJSpriteColor default (1)
    bool dontDraw = false;

    auto getColorFrame = [&] {
        if (sv.ends_with("_001.png")) {
            sv.remove_suffix(8);
            return std::string{sv} + "_color_001.png";
        }
        return std::string{sv};
    };

    auto replaceFirst = [&](std::string_view find, std::string_view replace) {
        std::string str{sv};
        auto pos = str.find(find);
        if (pos != std::string::npos) {
            str.replace(pos, find.size(), replace);
        }
        return str;
    };

    std::string name;
    switch (COLOR_SPRITE_MAP[index]) {
        case 0: name = getColorFrame(); break;
        case 1: name = "lightsquare_04_05_color_001.png"; break;
        case 2: name = replaceFirst("_00", "_color_00"); defaultChannel = 1012; break;
        case 3: name = std::string{sv}; dontDraw = true; break;
        case 4: name = replaceFirst("_001.png", "_color_001.png"); defaultChannel = 1006; break;
        case 5: name = getColorFrame(); defaultChannel = 1010; break;
        case 6: name = replaceFirst("_001.png", "_color_001.png"); defaultChannel = 1012; break;
        case 7: name = "waterfallAnim_color_007.png"; defaultChannel = 1012; break;
        case 8: name = replaceFirst("_00", "_color_00"); break;
        case 9: name = replaceFirst("_00", "_color_00"); defaultChannel = 2; break;
        case 10: name = replaceFirst("_001.png", "_color_001.png"); defaultChannel = 2; break;
        case 11: name = "pixelart_454_color_001.png"; defaultChannel = 2; break;
        case 12: name = replaceFirst("_001.png", "_2_001.png"); defaultChannel = 2; break;
        default: return;
    }

    auto colorSprite = CCSpriteExt::createFrameThreadSafe(name);
    colorSprite->setPosition(m_object->getPosition());
    m_object->m_colorSprite = colorSprite.take();

    if (!m_object->m_detailColor) {
        auto detailColor = new GJSpriteColor();
        detailColor->m_defaultColorID = 1;
        detailColor->m_colorID = 0;
        m_object->m_detailColor = detailColor;
    }

    if (dontDraw) {
        colorSprite->setDontDraw(true);
    }

    if (defaultChannel >= 0) {
        m_object->m_detailColor->m_defaultColorID = defaultChannel;
        m_object->m_detailColor->m_colorID = 0;
    }
}

void GameObjectBuilder::addGlow(std::string_view sv) {
    if (
        m_object->m_editorEnabled
        || (GameManager::get()->m_addGlow && !m_object->m_isInvisibleBlock)
        || m_object->m_isHide
        || !std::ranges::binary_search(GLOW_IDS, m_object->m_objectID)
    ) return;

    std::string glow{sv};
    auto it = glow.find("_001.png");
    if (it != std::string::npos) {
        glow.replace(it, 8, "_glow_001.png");
    }

    B_ASSERT(!m_object->m_glowSprite);
    auto spr = CCSpriteExt::createFrameThreadSafe(glow);
    spr->setPosition(m_object->getPosition());
    spr->setOpacity(255);
    m_object->m_glowSprite = spr.take();
}

void GameObjectBuilder::performFinalCleanup() {
    this->cleanup();
}

State GameObjectBuilder::state() const {
    return m_state.load(std::memory_order::relaxed);
}

void GameObjectBuilder::setState(State state) {
    m_state.store(state, std::memory_order::relaxed);
}

bool GameObjectBuilder::completed() const {
    auto s = this->state();
    return s == State::Skipped || s == State::Finished;
}

bool GameObjectBuilder::autoreleased() {
    return m_autoreleased;
}

}
