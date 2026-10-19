#include "objects.hpp"
#include <modules/engine/CCSprite.hpp>
#include <modules/engine/CCArray.hpp>
#include <arc/util/ScopeDtor.hpp>

using namespace geode::prelude;

namespace blaze {

template <typename T>
bool initGameObject(T* ret, CCSpriteFrame* frame) {
    static_assert(!std::is_same_v<T, T>, "don't know how to initialize this object");
}

template <typename T>
static Ref<GameObject> createGameObject(CCSpriteFrame* frame) {
    auto ret = new T();
    if (!initGameObject<T>(ret, frame)) return {};
    return Ref<GameObject>::adopt(ret);
}

static std::optional<std::string> frameForObjectId(int objectId) {
    auto ot = ObjectToolbox::sharedState();
    auto it = ot->m_allKeys.find(objectId);
    if (it == ot->m_allKeys.end()) return std::nullopt;
    return it->second;
}

// implementations

// cheeky impl for CCSprite::initWithSpriteFrameName, since default one is not 100% thread safe
template <>
bool initGameObject(CCSprite* obj, CCSpriteFrame* frame) {
    return static_cast<CCSpriteExt*>(obj)->initWithSpriteFrameThreadSafe(frame);
}

template <>
bool initGameObject(GameObject* obj, CCSpriteFrame* frame) {
    if (!frame) return false;

    // TODO: init is not safe for multithread due to setTexture
    obj->m_eObjType = cocos2d::CCObjectType::GameObject;
    if (!initGameObject<CCSprite>(obj, frame)) return false;
    obj->commonSetup();
    obj->m_bUnkBool2 = true;
    return true;
}

template <>
bool initGameObject(EffectGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<GameObject>(obj, frame)) return false; // technically inherits enhanced but needs nothing from it

    obj->m_bUnkBool2 = false;
    obj->m_classType = GameObjectClassType::Effect;
    obj->m_duration = 0.5f;
    obj->m_legacyHSV = true;
    obj->m_triggerTargetColor = {255, 255, 255};
    return obj;
}


template <>
bool initGameObject(EnhancedGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<GameObject>(obj, frame)) return false;

    obj->m_bUnkBool2 = false;
    obj->m_classType = GameObjectClassType::Enhanced;
    return true;
}

template <>
bool initGameObject(EnterEffectObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false;

    obj->m_objectType = GameObjectType::EnterEffectObject;
    obj->m_unk390 = 45;

    return true;
}

template <>
bool initGameObject(StartPosObject* obj, CCSpriteFrame* frame) {
    B_ASSERT(false && "StartPosObject is main thread only");
}

template <>
bool initGameObject(RingObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false;

    obj->m_customGlowColor = true;
    obj->m_isTouchTriggered = true;

    return true;
}

template <>
bool initGameObject(TeleportPortalObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(EnhancedTriggerObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(AnimatedGameObject* obj, CCSpriteFrame* frame) {
    B_ASSERT(false);
    return false;
}

template <>
bool initGameObject(SpawnTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify this is same as "edit_eSpawnBtn_001.png"
}

template <>
bool initGameObject(CountTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(LabelGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<GameObject>(obj, frame)) return false; // TODO: verify frame = "edit_eCounterBtn_001.png"

    obj->m_bUnkBool2 = false;

    return true;
}

template <>
bool initGameObject(TriggerControlGameObject* obj, CCSpriteFrame* frame) {
    // thanks i hate it
    obj->m_triggerControlFrame = frameForObjectId(1616).value();

    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(DashRingObject* obj, CCSpriteFrame* frame) {
    return initGameObject<RingObject>(obj, frame);
}

template <>
bool initGameObject(CameraTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(PlayerControlGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify frame == "edit_ePlayerControlBtn_001.png"
}

template <>
bool initGameObject(SFXTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(SongTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<SFXTriggerGameObject>(obj, frame);
}
template <>
bool initGameObject(SpecialAnimGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EnhancedGameObject>(obj, frame);
}

template <>
bool initGameObject(CheckpointGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false; // TODO: verify frame == "checkpoint_d_01_001.png"

    obj->m_objectType = GameObjectType::Modifier;
    obj->m_hasCustomAnimation = true;

    return true;
}

template <>
bool initGameObject(ParticleGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EnhancedGameObject>(obj, frame)) return false; // TODO: verify frame == "edit_eCParticleBtn_001.png"

    obj->m_hasCustomAnimation = true;

    return true;
}

template <>
bool initGameObject(TransformTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(ChanceTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(RandTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<ChanceTriggerGameObject>(obj, frame); // TODO: verify frame == "edit_eAdvRandomBtn_001.png"
}

template <>
bool initGameObject(ForceBlockGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(SmartGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<GameObject>(obj, frame)) return false;
    obj->m_classType = GameObjectClassType::Smart;
    return true;
}

template <>
bool initGameObject(GameOptionsTrigger* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(RotateGameplayGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify frame == "edit_eGameRotBtn_001.png"
}

template <>
bool initGameObject(GradientTriggerObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false; // TODO: verify frame == "edit_eGradientBtn_001.png"
    obj->m_blendingLayer = 1;
    return true;
}

template <>
bool initGameObject(ShaderGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(AdvancedFollowTriggerObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(ArtTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(KeyframeGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false; // TODO: verify frame == "keyframeIcon_001.png"

    // avoid plain create() due to autorelease
    obj->m_shadowObjects = ArrayExt::create().take();

    return true;
}

template <>
bool initGameObject(KeyframeAnimTriggerObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: confirm frame == "edit_eKeyframeBtn_001.png"
}

template <>
bool initGameObject(EndTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: confirm edit_eEndBtn_001.png
}

template <>
bool initGameObject(EventLinkTrigger* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify "edit_eEventLinkBtn_001.png"
}

template <>
bool initGameObject(SequenceTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<ChanceTriggerGameObject>(obj, frame); // TODO: verify "edit_eSequenceBtn_001.png"
}

template <>
bool initGameObject(SpawnParticleGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify "edit_eSpawnParticleBtn_001.png"
}

template <>
bool initGameObject(UISettingsGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify "edit_eUISettingsBtn_001.png"
}

template <>
bool initGameObject(TimerTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(ItemTriggerGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame);
}

template <>
bool initGameObject(AudioLineGuideGameObject* obj, CCSpriteFrame* frame) {
    if (!initGameObject<EffectGameObject>(obj, frame)) return false; // TODO: verify edit_eBPMBtn_001.png
    obj->m_duration = 60.0f;
    return true;
}

template <>
bool initGameObject(ObjectControlGameObject* obj, CCSpriteFrame* frame) {
    return initGameObject<EffectGameObject>(obj, frame); // TODO: verify "edit_eCollisionStateBtn_001.png"
}

template <>
bool initGameObject(AdvancedFollowEditObject* obj, CCSpriteFrame* frame) {
    return initGameObject<AdvancedFollowTriggerObject>(obj, frame);
}

static auto createRingEffectObject(CCSpriteFrame* frame) {
    auto ret = createGameObject<RingObject>(frame);
    ret->m_classType = GameObjectClassType::Effect;
    return ret;
}

static auto createTeleportObject(CCSpriteFrame* frame, bool trigger) {
    auto obj = createGameObject<TeleportPortalObject>(frame);
    if (!trigger) {
        static_cast<TeleportPortalObject*>(obj.data())->m_isTouchTriggered = true;
    }
    return obj;
}

static Ref<GameObject> createTextGameObject(CCSpriteFrame*) {
    // XXX (2.209): new fonts may be added
    // int font = std::clamp(GameManager::get()->m_loadedFont, 1, 59);
    B_ASSERT(false);
    return nullptr;
}

static auto createSmartObject(CCSpriteFrame* frame, int objectId) {
    auto obj = createGameObject<SmartGameObject>(frame);
    static_cast<SmartGameObject*>(obj.data())->m_smartFrame = frameForObjectId(objectId).value();
    return obj;
}

static Ref<GameObject> createGameObject(int objectId, CCSpriteFrame* frame, bool* needsCustomSetupP) {
    // hack xd
    bool needsCustomSetup = true;
    auto _ = arc::scopeDtor([&] {
        if (needsCustomSetupP) *needsCustomSetupP = needsCustomSetup;
    });

    switch (objectId) {
        // -- EffectGameObject --
        case 10 ... 13:
        case 29:
        case 30:
        case 32:
        case 33:
        case 35:
        case 45 ... 47:
        case 67:
        case 99:
        case 101:
        case 105:
        case 111:
        case 140:
        case 142:
        case 200 ... 203:
        case 286 ... 287:
        case 660:
        case 744 ... 745:
        case 899 ... 900:
        case 915:
        case 1006 ... 1007:
        case 1049:
        case 1275:
        case 1329:
        case 1331 ... 1332:
        case 1334:
        case 1347:
        case 1520:
        case 1585:
        case 1587:
        case 1589:
        case 1595:
        case 1598:
        case 1612 ... 1614:
        case 1755:
        case 1812 ... 1816:
        case 1818 ... 1819:
        case 1829:
        case 1859:
        case 1912 ... 1913:
        case 1917:
        case 1931:
        case 1933:
        case 1935:
        case 2066:
        case 2866:
        case 2901:
        case 2926:
        case 2999:
        case 3005:
        case 3601:
        case 3606:
        case 3609:
        case 3612:
        case 3618:
        case 3640:
        case 3662:
        case 4401 ... 4539: {
            return createGameObject<EffectGameObject>(frame);
        } break;

        // -- EnhancedGameObject --
        case 85 ... 89:
        case 97 ... 98:
        case 137 ... 139:
        case 154 ... 156:
        case 180 ... 188:
        case 222 ... 224:
        case 375 ... 378:
        case 394 ... 399:
        case 675 ... 680:
        case 740 ... 742:
        case 920 ... 921:
        case 923 ... 924:
        case 997 ... 1000:
        case 1019 ... 1021:
        case 1050 ... 1061:
        case 1516:
        case 1518 ... 1519:
        case 1521 ... 1528:
        case 1582 ... 1583:
        case 1591 ... 1593:
        case 1618 ... 1620:
        case 1697 ... 1699:
        case 1705 ... 1710:
        case 1734 ... 1736:
        case 1752:
        case 1831 ... 1834:
        case 1839 ... 1842:
        case 1849 ... 1858:
        case 1860:
        case 1936 ... 1939:
        case 2020 ... 2045:
        case 2048 ... 2054:
        case 2223:
        case 2246:
        case 2605:
        case 2629 ... 2630:
        case 2694:
        case 2864 ... 2865:
        case 2867 ... 2894:
        case 3000 ... 3002:
        case 3119 ... 3121:
        case 3219:
        case 3303 ... 3304:
        case 3482 ... 3484:
        case 3492 ... 3493:
        case 4211:
        case 4300: {
            return createGameObject<EnhancedGameObject>(frame);
        } break;

        // -- EnterEffectObject --
        case 22 ... 28:
        case 55 ... 59:
        case 1915:
        case 3006 ... 3015:
        case 3017 ... 3021:
        case 3023 ... 3024: {
            return createGameObject<EnterEffectObject>(frame);
        } break;

        // -- StartPosObject --
        case 31: {
            return createGameObject<StartPosObject>(frame);
        } break;

        // -- RingObject --
        case 36:
        case 84:
        case 141:
        case 1022:
        case 1330:
        case 1333:
        case 3004: {
            return createGameObject<RingObject>(frame);
        } break;


        // -- RingObject but with type 1 (effect) ? --
        case 1594:
        case 3643: {
            return createRingEffectObject(frame);
        } break;

        // -- TeleportObject --
        case 747: case 749:
        case 2064:
        case 2902:
        case 3027: {
            return createTeleportObject(frame, false);
        } break;


        // -- TeleportObject but no touch trigger --
        case 3022: {
            return createTeleportObject(frame, true);
        } break;

        // -- EnhancedTrigger --
        case 901: case 1346: {
            return createGameObject<EnhancedTriggerObject>(frame);
        } break;


        // -- TextGameObject --
        case 914: {
            return createTextGameObject(frame);
        } break;

        // -- AnimatedGameObject --
        case 918 ... 919:
        case 1327 ... 1328:
        case 1584:
        case 2012: {
            return createGameObject<AnimatedGameObject>(frame);
        } break;

        // -- SpawnTriggerGameObject --
        case 1268: {
            return createGameObject<SpawnTriggerGameObject>(frame);
        } break;

        // -- CountTriggerGameObject --
        case 1611:
        case 1811:
        case 1817: {
            return createGameObject<CountTriggerGameObject>(frame);
        } break;

        // -- LabelGameObject --
        case 1615: {
            return createGameObject<LabelGameObject>(frame);
        } break;

        // -- TriggerControlGameObject --
        case 1616: {
            return createGameObject<TriggerControlGameObject>(frame);
        } break;

        // -- DashRingObject --
        case 1704:
        case 1751: {
            return createGameObject<DashRingObject>(frame);
        } break;

        // -- CameraTriggerGameObject --
        case 1914:
        case 1916:
        case 2015 ... 2016:
        case 2062:
        case 2925: {
            return createGameObject<CameraTriggerGameObject>(frame);
        } break;

        // -- PlayerControlGameObject --
        case 1932: {
            return createGameObject<PlayerControlGameObject>(frame);
        } break;

        // -- SongTriggerGameObject --
        case 1934:
        case 3605: {
            return createGameObject<SongTriggerGameObject>(frame);
        } break;

        // -- SpecialAnimGameObject --
        case 2046 ... 2047:
        case 2055: {
            return createGameObject<SpecialAnimGameObject>(frame);
        } break;

        // -- CheckpointGameObject --
        case 2063: {
            return createGameObject<CheckpointGameObject>(frame);
        } break;

        // -- ParticleGameObject --
        case 2065: {
            return createGameObject<ParticleGameObject>(frame);
        } break;

        // -- TransformTriggerGameObject --
        case 2067: {
            return createGameObject<TransformTriggerGameObject>(frame);
        } break;

        // -- RandTriggerGameObject --
        case 2068: {
            return createGameObject<RandTriggerGameObject>(frame);
        } break;

        // -- ForceBlockGameObject --
        case 2069:
        case 3645: {
            return createGameObject<ForceBlockGameObject>(frame);
        } break;

        // -- SmartGameObject --
        case 2895 ... 2897: {
            return createSmartObject(frame, objectId);
        } break;

        // -- GameOptionsTrigger --
        case 2899: {
            return createGameObject<GameOptionsTrigger>(frame);
        } break;

        // -- RotateGameplayGameObject --
        case 2900: {
            return createGameObject<RotateGameplayGameObject>(frame);
        } break;

        // -- GradientTriggerObject --
        case 2903: {
            return createGameObject<GradientTriggerObject>(frame);
        } break;

        // -- ShaderGameObject --
        case 2904 ... 2905:
        case 2907:
        case 2909 ... 2917:
        case 2919 ... 2924: {
            return createGameObject<ShaderGameObject>(frame);
        } break;

        // -- AdvancedFollowTriggerObject --
        case 3016: {
            return createGameObject<AdvancedFollowTriggerObject>(frame);
        } break;

        // -- ArtTriggerGameObject --
        case 3029 ... 3031: {
            return createGameObject<ArtTriggerGameObject>(frame);
        } break;

        // -- KeyframeGameObject --
        case 3032: {
            return createGameObject<KeyframeGameObject>(frame);
        } break;

        // -- KeyframeAnimTriggerObject --
        case 3033: {
            return createGameObject<KeyframeAnimTriggerObject>(frame);
        } break;

        // -- EndTriggerGameObject --
        case 3600: {
            return createGameObject<EndTriggerGameObject>(frame);
        } break;

        // -- SFXTriggerGameObject --
        case 3602 ... 3603: {
            return createGameObject<SFXTriggerGameObject>(frame);
        } break;

        // -- EventLinkTrigger --
        case 3604: {
            return createGameObject<EventLinkTrigger>(frame);
        } break;

        // -- SequenceTriggerGameObject --
        case 3607: {
            return createGameObject<SequenceTriggerGameObject>(frame);
        } break;

        // -- SpawnParticleGameObject --
        case 3608: {
            return createGameObject<SpawnParticleGameObject>(frame);
        } break;

        // -- UISettingsGameObject --
        case 3613: {
            return createGameObject<UISettingsGameObject>(frame);
        } break;

        // -- TimerTriggerGameObject --
        case 3614 ... 3615:
        case 3617: {
            return createGameObject<TimerTriggerGameObject>(frame);
        } break;

        // -- ItemTriggerGameObject --
        case 3619 ... 3620:
        case 3641: {
            return createGameObject<ItemTriggerGameObject>(frame);
        } break;

        // -- AudioLineGuideGameObject --
        case 3642: {
            return createGameObject<AudioLineGuideGameObject>(frame);
        } break;

        // -- ObjectControlGameObject --
        case 3655: {
            return createGameObject<ObjectControlGameObject>(frame);
        } break;

        // -- AdvancedFollowEditObject --
        case 3660 ... 3661: {
            return createGameObject<AdvancedFollowEditObject>(frame);
        } break;

        // Fallback: assume any other object is just a regular gameobject
        default: {
            needsCustomSetup = false;
            return createGameObject<GameObject>(frame);
        } break;
    }
}

template <int GDVer, size_t Off, size_t Alt>
struct Offset {
    static constexpr size_t value = Off;
    static constexpr size_t alt = Alt;
    static constexpr bool valid = (GEODE_COMP_GD_VERSION == GDVer);

    template <typename T>
    T* addr(bool alt = false) const {
        return (T*)(base::get() + (alt ? this->alt : this->value));
    }
};

static constexpr
    GEODE_WINDOWS  (Offset<22081, 0x6ba158, 0x0>)
g_objectIdOffset;

static constexpr
    GEODE_WINDOWS  (Offset<22081, 0x6c2ee0, 0x0>)
g_varianceOffset;

int& getUniqueGameObjectId() {
    return *g_objectIdOffset.addr<int>();
}

uint64_t& getVarianceSeed() {
    return *g_varianceOffset.addr<uint64_t>();
}

}