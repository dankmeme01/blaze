#include "tracing.hpp"

#ifdef BLAZE_TRACY

#include <TracyOpenGL.hpp>
class GLFWwindow;
#include <Geode/Modify.hpp>

using namespace geode::prelude;

// A lot of this was taken from algebradash <3

#ifdef GEODE_IS_WINDOWS
class $modify(CCEGLView) {
    bool initGlew() {
        ZoneScopedN("CCEGLView::initGlew");
        if (!CCEGLView::initGlew()) return false;

        TracyGpuContext;

        return true;
    }
    void swapBuffers() {
        ZoneScopedN("CCEGLView::swapBuffers");
        CCEGLView::swapBuffers();
        FrameMark;
    }

    static CCEGLView* createWithRect(const gd::string& p0, CCRect p1, float p2) {
        ZoneScopedN("CCEGLView::createWithRect");

        return CCEGLView::createWithRect(p0, p1, p2);
    }

    static CCEGLView* createWithFullScreen(const gd::string& p0, bool p1) {
        ZoneScopedN("CCEGLView::createWithFullscreen");

        return CCEGLView::createWithFullScreen(p0, p1);
    }

    void setupWindow(CCRect r) {
        ZoneScopedN("CCEGLView::setupWindow");

        return CCEGLView::setupWindow(r);
    }
};
#endif

#define PROFILER_HOOK_3(Ret_, Class_, Name_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_() { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(); } };
#define PROFILER_HOOK_4(Ret_, Class_, Name_, A_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a); } };
#define PROFILER_HOOK_5(Ret_, Class_, Name_, A_, B_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b); } };
#define PROFILER_HOOK_6(Ret_, Class_, Name_, A_, B_, C_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c); } };
#define PROFILER_HOOK_7(Ret_, Class_, Name_, A_, B_, C_, D_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d); } };
#define PROFILER_HOOK_8(Ret_, Class_, Name_, A_, B_, C_, D_, E_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e); } };
#define PROFILER_HOOK_9(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f); } };
#define PROFILER_HOOK_10(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_, G_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f, G_ g) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f, g); } };
#define PROFILER_HOOK_11(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_, G_, H_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f, G_ g, H_ h) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f, g, h); } };
#define PROFILER_HOOK_12(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_, G_, H_, I_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f, G_ g, H_ h, I_ i) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f, g, h, i); } };
#define PROFILER_HOOK_13(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_, G_, H_, I_, J_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f, G_ g, H_ h, I_ i, J_ j) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f, g, h, i, j); } };
#define PROFILER_HOOK_14(Ret_, Class_, Name_, A_, B_, C_, D_, E_, F_, G_, H_, I_, J_, K_) class GEODE_CRTP2(GEODE_CONCAT(profilerHook, __LINE__), Class_) { \
    Ret_ Name_(A_ a, B_ b, C_ c, D_ d, E_ e, F_ f, G_ g, H_ h, I_ i, J_ j, K_ k) { ZoneScopedN(#Class_ "::" #Name_); return Class_::Name_(a, b, c, d, e, f, g, h, i, j, k); } };

#define PROFILER_HOOK(...) GEODE_INVOKE(GEODE_CONCAT(PROFILER_HOOK_, GEODE_NUMBER_OF_ARGS(__VA_ARGS__)), __VA_ARGS__)


GEODE_WINDOWS(PROFILER_HOOK(void, CCEGLView, pollEvents));
PROFILER_HOOK(void, CCDirector, drawScene);
PROFILER_HOOK(void, CCDirector, setNextScene);
PROFILER_HOOK(void, CCDirector, setOpenGLView, CCEGLView*);

// gd init
PROFILER_HOOK(bool, EditLevelLayer, init, GJGameLevel*)
PROFILER_HOOK(bool, EditorUI, init, LevelEditorLayer*)
PROFILER_HOOK(bool, LevelEditorLayer, init, GJGameLevel*, bool)
PROFILER_HOOK(bool, LoadingLayer, init, bool)
PROFILER_HOOK(bool, MenuLayer, init)
PROFILER_HOOK(bool, PlayerObject, init, int, int, GJBaseGameLayer*, CCLayer*, bool)
PROFILER_HOOK(bool, PlayLayer, init, GJGameLevel*, bool, bool)

// gd update
PROFILER_HOOK(void, DrawGridLayer, update, float)
PROFILER_HOOK(void, FMODAudioEngine, update, float)
PROFILER_HOOK(void, GameManager, update, float)
PROFILER_HOOK(void, LevelEditorLayer, update, float)
PROFILER_HOOK(void, PlayerObject, update, float)
PROFILER_HOOK(void, PlayLayer, update, float)

PROFILER_HOOK(void, GJEffectManager, updatePulseEffects, float)
PROFILER_HOOK(void, PlayLayer, updateCamera, float)
PROFILER_HOOK(void, PlayLayer, updateVisibility, float)
PROFILER_HOOK(void, PlayLayer, updateLevelColors)
PROFILER_HOOK(void, PlayLayer, updateProgressbar)
PROFILER_HOOK(void, GameObject, setObjectColor, const ccColor3B&)

PROFILER_HOOK(void, GameObject, updateOrientedBox)
PROFILER_HOOK(void, GJBaseGameLayer, removeObjectFromSection, GameObject*)
PROFILER_HOOK(int, GJBaseGameLayer, checkCollisions, PlayerObject*, float, bool);
PROFILER_HOOK(void, GJBaseGameLayer, visit);
PROFILER_HOOK(void, GJBaseGameLayer, addKeyframe, KeyframeGameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, addToGroup, GameObject*, int, bool);
PROFILER_HOOK(void, GJBaseGameLayer, addToGroups, GameObject*, bool);
PROFILER_HOOK(void, GJBaseGameLayer, addToSection, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, bumpPlayer, PlayerObject*, EffectGameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, checkCollisionBlocks, EffectGameObject*, gd::vector<EffectGameObject*>*, int);
PROFILER_HOOK(int, GJBaseGameLayer, checkCollisions, PlayerObject*, float, bool);
PROFILER_HOOK(void, GJBaseGameLayer, checkpointActivated, CheckpointGameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, checkSpawnObjects);
PROFILER_HOOK(void, GJBaseGameLayer, collisionCheckObjects, PlayerObject*, gd::vector<GameObject*>*, int, float);
PROFILER_HOOK(void, GJBaseGameLayer, createGroundLayer, int, int);
PROFILER_HOOK(void, GJBaseGameLayer, createPlayer);
PROFILER_HOOK(void, GJBaseGameLayer, destroyObject, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, gameEventTriggered, GJGameEvent, int, int);
PROFILER_HOOK(float, GJBaseGameLayer, getEasedAreaValue, GameObject*, EnterEffectInstance*, float, bool, int);
PROFILER_HOOK(bool, GJBaseGameLayer, init);
PROFILER_HOOK(void, GJBaseGameLayer, loadUpToPosition, float, int, int);
PROFILER_HOOK(void, GJBaseGameLayer, moveAreaObject, GameObject*, float, float);
PROFILER_HOOK(void, GJBaseGameLayer, objectsCollided, int, int);
PROFILER_HOOK(void, GJBaseGameLayer, pickupItem, EffectGameObject*);
PROFILER_HOOK(bool, GJBaseGameLayer, playerCircleCollision, PlayerObject*, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, playerTouchedRing, PlayerObject*, RingObject*);
PROFILER_HOOK(void, GJBaseGameLayer, playerTouchedTrigger, PlayerObject*, EffectGameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, playerWillSwitchMode, PlayerObject*, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, processAdvancedFollowActions, float);
PROFILER_HOOK(void, GJBaseGameLayer, processAreaActions, float, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processAreaEffects, gd::vector<EnterEffectInstance>*, GJAreaActionType, float, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processAreaMoveGroupAction, cocos2d::CCArray*, EnterEffectInstance*, cocos2d::CCPoint, int, int, int, int, int, bool, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processAreaRotateGroupAction, cocos2d::CCArray*, EnterEffectInstance*, cocos2d::CCPoint, int, int, int, int, int, bool, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processAreaTransformGroupAction, cocos2d::CCArray*, EnterEffectInstance*, cocos2d::CCPoint, int, int, int, int, int, bool, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processCommands, float);
PROFILER_HOOK(void, GJBaseGameLayer, processDynamicObjectActions, int, float);
PROFILER_HOOK(void, GJBaseGameLayer, processFollowActions);
PROFILER_HOOK(void, GJBaseGameLayer, processMoveActions);
PROFILER_HOOK(void, GJBaseGameLayer, processMoveActionsStep, float, bool);
PROFILER_HOOK(void, GJBaseGameLayer, processPlayerFollowActions, float);
PROFILER_HOOK(void, GJBaseGameLayer, processQueuedAudioTriggers);
PROFILER_HOOK(void, GJBaseGameLayer, processQueuedButtons);
PROFILER_HOOK(void, GJBaseGameLayer, processRotationActions);
PROFILER_HOOK(void, GJBaseGameLayer, processTransformActions, bool);
PROFILER_HOOK(void, GJBaseGameLayer, removeFromGroup, GameObject*, int);
PROFILER_HOOK(void, GJBaseGameLayer, removeObjectFromSection, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, resetAudio);
PROFILER_HOOK(int, GJBaseGameLayer, resetAreaObjectValues, GameObject*, bool);
PROFILER_HOOK(float, GJBaseGameLayer, getAreaObjectValue, EnterEffectInstance*, GameObject*, cocos2d::CCPoint&, bool&);
PROFILER_HOOK(void, GJBaseGameLayer, resetCamera);
PROFILER_HOOK(void, GJBaseGameLayer, resetPlayer);
PROFILER_HOOK(void, GJBaseGameLayer, rotateGameplay, RotateGameplayGameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, setupLayers);
PROFILER_HOOK(void, GJBaseGameLayer, sortSectionVector);
PROFILER_HOOK(void, GJBaseGameLayer, spawnGroup, int, bool, double, gd::vector<int> const&, int, int);
PROFILER_HOOK(void, GJBaseGameLayer, spawnObject, GameObject*, double, gd::vector<int> const&);
PROFILER_HOOK(void, GJBaseGameLayer, teleportPlayer, TeleportPortalObject*, PlayerObject*);
PROFILER_HOOK(void, GJBaseGameLayer, toggleFlipped, bool, bool);
PROFILER_HOOK(void, GJBaseGameLayer, update, float);
PROFILER_HOOK(void, GJBaseGameLayer, updateCamera, float);
PROFILER_HOOK(void, GJBaseGameLayer, updateCollisionBlocks);
PROFILER_HOOK(void, GJBaseGameLayer, updateLevelColors);
PROFILER_HOOK(void, GJBaseGameLayer, updateShaderLayer, float);
PROFILER_HOOK(void, GJBaseGameLayer, updateObjectSection, GameObject*);
PROFILER_HOOK(void, GJBaseGameLayer, updateTimeWarp, GameObject*, float);
PROFILER_HOOK(void, GJBaseGameLayer, updateTimeWarp, float);
PROFILER_HOOK(void, GJBaseGameLayer, updateZoom, float, float, int, float, int, int);

PROFILER_HOOK(void, GameObject, activateObject)
PROFILER_HOOK(void, GameObject, activateObject)
PROFILER_HOOK(void, GameObject, deactivateObject, bool)

PROFILER_HOOK(bool, GameManager, getGameVariable, const char*)
PROFILER_HOOK(void, GameManager, setGameVariable, const char*, bool)

// gd draw
PROFILER_HOOK(void, DrawGridLayer, draw)
PROFILER_HOOK(void, EditorUI, draw)

// loading
PROFILER_HOOK(void, LoadingLayer, loadAssets)
PROFILER_HOOK(CCTexture2D*, CCTextureCache, addImage, const char*, bool)
PROFILER_HOOK(void, CCSpriteFrameCache, addSpriteFramesWithFile, const char*, const char*)
PROFILER_HOOK(void, CCSpriteFrameCache, addSpriteFramesWithFile, const char*)
PROFILER_HOOK(void, CCSpriteFrameCache, addSpriteFramesWithFile, const char*, CCTexture2D*)
PROFILER_HOOK(CCLabelBMFont*, CCLabelBMFont, create, const char*, const char*, float, CCTextAlignment, CCPoint)
PROFILER_HOOK(bool, GameManager, init)
PROFILER_HOOK(bool, GameLevelManager, init);
PROFILER_HOOK(bool, CCTexture2D, initWithImage, CCImage*)
PROFILER_HOOK(bool, CCTexture2D, initWithData, const void*, CCTexture2DPixelFormat, unsigned int, unsigned int, const CCSize&);
PROFILER_HOOK(bool, CCSpriteBatchNode, initWithTexture, CCTexture2D*, unsigned int)
PROFILER_HOOK(bool, CCSprite, initWithTexture, CCTexture2D*, const CCRect&, bool)

PROFILER_HOOK(void, CCScheduler, update, float)

// actions
PROFILER_HOOK(void, CCActionManager, update, float)
PROFILER_HOOK(void, CCCallFunc, update, float)
PROFILER_HOOK(void, CCSequence, update, float)

PROFILER_HOOK(void, CCBlink, update, float)
PROFILER_HOOK(void, CCFadeIn, update, float)
PROFILER_HOOK(void, CCFadeOut, update, float)
PROFILER_HOOK(void, CCFadeTo, update, float)
PROFILER_HOOK(void, CCMoveBy, update, float)
PROFILER_HOOK(void, CCRepeat, update, float)
PROFILER_HOOK(void, CCRotateBy, update, float)
PROFILER_HOOK(void, CCScaleTo, update, float)
PROFILER_HOOK(void, CCTintTo, update, float)

// progress
PROFILER_HOOK(void, CCProgressTimer, updateBar)
PROFILER_HOOK(void, CCProgressTimer, updateProgress)
PROFILER_HOOK(void, CCProgressTimer, updateRadial)

// easings
PROFILER_HOOK(void, CCEaseBackOut, update, float)

PROFILER_HOOK(void, CCEaseBounceOut, update, float)
PROFILER_HOOK(void, CCEaseElasticOut, update, float)
PROFILER_HOOK(void, CCEaseExponentialOut, update, float)
PROFILER_HOOK(void, CCEaseIn, update, float)
PROFILER_HOOK(void, CCEaseInOut, update, float)
PROFILER_HOOK(void, CCEaseOut, update, float)
PROFILER_HOOK(void, CCEaseSineIn, update, float)
PROFILER_HOOK(void, CCEaseSineInOut, update, float)
PROFILER_HOOK(void, CCEaseSineOut, update, float)
// particles
PROFILER_HOOK(void, CCParticleSystem, update, float)
PROFILER_HOOK(void, CCParticleSystem, updateWithNoTime)
PROFILER_HOOK(void, CCParticleSystemQuad, draw)

// sprites
PROFILER_HOOK(void, CCSprite, setOpacity, GLubyte)
PROFILER_HOOK(void, CCSprite, setColor, const ccColor3B&)
PROFILER_HOOK(void, CCSprite, setOpacityModifyRGB, bool)
PROFILER_HOOK(void, CCSprite, updateDisplayedColor, const ccColor3B&)
PROFILER_HOOK(void, CCSprite, updateDisplayedOpacity, GLubyte)
PROFILER_HOOK(void, CCSprite, updateColor)
PROFILER_HOOK(void, CCSprite, updateTransform)
PROFILER_HOOK(void, CCSprite, draw)
PROFILER_HOOK(CCSpriteFrame*, CCSprite, displayFrame)
PROFILER_HOOK(void, CCSprite, sortAllChildren)
PROFILER_HOOK(void, CCSpriteBatchNode, draw)
PROFILER_HOOK(void, CCSpriteBatchNode, sortAllChildren)
 // TODO private // PROFILER_HOOK(void, CCSpriteBatchNode, updateAtlastIndex, CCSprite*, int*)
 // TODO private // PROFILER_HOOK(void, CCSpriteBatchNode, swap, int, int)

PROFILER_HOOK(void, CCLabelBMFont, setString, const char*, bool)
PROFILER_HOOK(void, CCLabelBMFont, updateLabel)

PROFILER_HOOK(void, CCNode, update, float)
PROFILER_HOOK(void, CCNode, visit)
PROFILER_HOOK(void, CCNode, updateTransform)

PROFILER_HOOK(int, CCApplication, run)
GEODE_WINDOWS(PROFILER_HOOK(void, AppDelegate, setupGLView))


#endif // BLAZE_TRACY