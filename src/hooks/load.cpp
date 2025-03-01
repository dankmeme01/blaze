// Blaze LoadingLayer and CCApplication/AppDelegate hooks.
//
// Completely rewrites the loading logic to be multithreaded.

#include "load.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>

#ifdef GEODE_IS_WINDOWS
# include <Geode/modify/CCApplication.hpp>
#elif defined(GEODE_IS_ANDROID)
# include <Geode/modify/AppDelegate.hpp>
#endif

#include <asp/sync/Mutex.hpp>
#include <asp/thread/ThreadPool.hpp>
#include <asp/thread/Thread.hpp>
#include <asp/time/Instant.hpp>
#include <asp/time/Duration.hpp>

#include <hooks/FMODAudioEngine.hpp>
#include <TaskTimer.hpp>
#include <manager.hpp>
#include <ccimageext.hpp>
#include <settings.hpp>
#include <tracing.hpp>
#include <util/hash.hpp>

#include "load/glfw.hpp"
#include "load/spriteframes.hpp"


using namespace geode::prelude;
using namespace asp::time;

// mutexes for thread unsafe classes
static asp::Mutex<> texCacheMutex, sfcacheMutex;
static GameManager* gm;

// AsyncImageLoadRequest - structure that handles the loading of a specific texture
namespace {
struct AsyncImageLoadRequest {
    const char* pngFile = nullptr;
    const char* plistFile = nullptr;
    gd::string pathKey;
    blaze::OwnedMemoryChunk imageData{};
    Ref<CCImage> image = nullptr;
    Ref<CCTexture2D> texture = nullptr;

    AsyncImageLoadRequest(const char* pngFile, const char* plistFile) : pngFile(pngFile), plistFile(plistFile) {}
    AsyncImageLoadRequest(const char* pngFile) : pngFile(pngFile), plistFile(nullptr) {}

    AsyncImageLoadRequest(const AsyncImageLoadRequest&) = delete;
    AsyncImageLoadRequest& operator=(const AsyncImageLoadRequest&) = delete;

    AsyncImageLoadRequest(AsyncImageLoadRequest&& other) {
        this->pngFile = other.pngFile;
        this->plistFile = other.plistFile;
        this->pathKey = std::move(other.pathKey);
        this->imageData = std::move(other.imageData);
        this->image = std::move(other.image);
        this->texture = std::move(other.texture);

        other.pngFile = nullptr;
        other.plistFile = nullptr;
    }

    AsyncImageLoadRequest& operator=(AsyncImageLoadRequest&& other) {
        if (this != &other) {
            this->pngFile = other.pngFile;
            this->plistFile = other.plistFile;
            this->pathKey = std::move(other.pathKey);
            this->imageData = std::move(other.imageData);
            this->image = std::move(other.image);
            this->texture = std::move(other.texture);

            other.pngFile = nullptr;
            other.plistFile = nullptr;
        }

        return *this;
    }

    // Loads the encoded image data into memory. Does nothing if the image is already loaded.
    inline Result<> loadImage() {
        ZoneScoped;

        if (imageData) return Ok();

        this->pathKey = CCFileUtils::get()->fullPathForFilename(pngFile, false);
        if (pathKey.empty()) {
            return Err(fmt::format("Failed to find path for image {}", pngFile));
        }

        log::debug("loadImage {}", pathKey);

        this->imageData = LoadManager::get().readFileToChunk(pathKey.c_str());

        if (!imageData || imageData.size == 0) {
            return Err(fmt::format("Failed to open image file at {}", pathKey));
        }

        return Ok();
    }

    bool isImageLoaded() {
        return (bool) this->imageData;
    }

    inline Result<> initImage() {
        ZoneScoped;

        if (!imageData) return Err("attempting to initialize an image that hasn't been loaded yet");

        this->image = new CCImage();
        this->image->release(); // make refcount go to 1
        auto ret = static_cast<blaze::CCImageExt*>(this->image.data())->initWithSPNGOrCache(imageData, pathKey.c_str());

        if (!ret) {
            this->image = nullptr;
            return Err(fmt::format("Failed to load image: {}", ret.unwrapErr()));
        }

        return Ok();
    }

    bool isImageInitialized() {
        return this->image != nullptr;
    }

    // Initializes the opengl texture from the loaded image. Must be called on the main thread
    inline Result<> initTexture() {
        ZoneScoped;

        if (!image) return Err("attempting to initialize a texture before initializing an image");

        this->texture = new CCTexture2D();
        this->texture->release(); // make refcount go to 1

        if (!texture->initWithImage(image)) {
            this->image = nullptr;
            this->texture = nullptr;
            return Err("failed to initialize cctexture2d");;
        }

        auto _lck = texCacheMutex.lock();
        CCTextureCache::get()->m_pTextures->setObject(texture, pathKey);

        return Ok();
    }

    bool isTextureLoaded() {
        return this->texture != nullptr;
    }

    // Async version of addSpriteFramesSync
    inline void addSpriteFrames(TextureQuality texQuality) const {
        if (!plistFile) return;
        if (!texture) return;

        ZoneScoped;

        // try to guess the full plist name without calling fullPathForFilename
        auto pfsv = std::string_view(plistFile);

        // strip .plist extension
        std::string plistPath;

        plistPath.reserve(pfsv.size() - sizeof(".plist") + sizeof("-uhd.plist"));
        plistPath.append(pfsv.substr(0, pfsv.size() - sizeof(".plist")));

        switch (texQuality) {
            case cocos2d::kTextureQualityLow:
                plistPath.append(std::string_view(".plist")); break;
            case cocos2d::kTextureQualityMedium:
                // @geode-ignore(unknown-resource)
                plistPath.append(std::string_view("-hd.plist")); break;
            case cocos2d::kTextureQualityHigh:
                // @geode-ignore(unknown-resource)
                plistPath.append(std::string_view("-uhd.plist")); break;
        }

        log::debug("loading plist {}", plistPath);

        {
            ZoneScopedN("addSpriteFrames loading plist");

            auto fu = CCFileUtils::get();
            unsigned long size = 0;
            unsigned char* dataptr = fu->getFileData(plistPath.c_str(), "rt", &size);

            if (!dataptr || size == 0) {
                delete[] dataptr;
                dataptr = nullptr;

                // Try another path
                plistPath = CCFileUtils::get()->fullPathForFilename(plistFile, false);
                dataptr = fu->getFileData(plistPath.c_str(), "rt", &size);

                log::debug("failed to load, loading another plist: {}", plistPath);

                if (!dataptr || size == 0) {
                    delete[] dataptr;
                    dataptr = nullptr;

                    log::warn("failed to find the plist for {}", plistFile);
                    auto _mtx = texCacheMutex.lock();
                    CCTextureCache::get()->m_pTextures->removeObjectForKey(pathKey);
                    return;
                }
            }

            std::unique_ptr<blaze::SpriteFrameData> spf;
            {
                ZoneScopedN("addSpriteFrames parsing sprite frames");
                auto res = blaze::parseSpriteFrames(dataptr, size);

                if (res) {
                    spf = std::move(res).unwrap();
                } else {
                    log::warn("failed to parse sprite frames for {}: {}", plistFile, res.unwrapErr());
                }
            }

            if (spf) {
                ZoneScopedN("addSpriteFrames adding frames to cache")

                auto _lck = sfcacheMutex.lock();

                blaze::addSpriteFrames(*spf, texture);
            }

            delete[] dataptr;
        }
    }
};
}

#define MAKE_SHEET(name) textures.push_back(AsyncImageLoadRequest { name".png", name".plist" })
#define MAKE_IMG(name) textures.push_back(AsyncImageLoadRequest { name".png" })
#define MAKE_FONT(name) do { \
        auto conf = FNTConfigLoadFile(name".fnt"); \
        if (conf) textures.push_back(AsyncImageLoadRequest { conf->getAtlasName() }); \
        else geode::log::warn("Failed to load font file: " name ".fnt"); \
    } while (0)

static std::vector<AsyncImageLoadRequest> getLoadingLayerResources() {
    std::vector<AsyncImageLoadRequest> textures;
    MAKE_SHEET("GJ_LaunchSheet");
    MAKE_IMG("game_bg_01_001");
    MAKE_IMG("slidergroove");
    MAKE_IMG("sliderBar");
    MAKE_FONT("goldFont");
    return textures;
}

static std::vector<AsyncImageLoadRequest> getGameResources() {
    std::vector<AsyncImageLoadRequest> textures;
    MAKE_SHEET("GJ_GameSheet");
    MAKE_SHEET("GJ_GameSheet02");
    MAKE_SHEET("GJ_GameSheet03");
    MAKE_SHEET("GJ_GameSheetEditor");
    MAKE_SHEET("GJ_GameSheet04");
    MAKE_SHEET("GJ_GameSheetGlow");

    MAKE_SHEET("FireSheet_01");
    MAKE_SHEET("GJ_ShopSheet");
    MAKE_IMG("smallDot");
    MAKE_IMG("square02_001");
    MAKE_SHEET("GJ_ParticleSheet");
    MAKE_SHEET("PixelSheet_01");

    MAKE_SHEET("CCControlColourPickerSpriteSheet");
    MAKE_IMG("GJ_gradientBG");
    MAKE_IMG("edit_barBG_001");
    MAKE_IMG("GJ_button_01");
    MAKE_IMG("slidergroove2");

    MAKE_IMG("GJ_square01");
    MAKE_IMG("GJ_square02");
    MAKE_IMG("GJ_square03");
    MAKE_IMG("GJ_square04");
    MAKE_IMG("GJ_square05");
    MAKE_IMG("gravityLine_001");

    MAKE_FONT("bigFont");
    MAKE_FONT("chatFont");
    return textures;
}
#undef MAKE_SHEET
#undef MAKE_IMG
#undef MAKE_FONT

static Instant g_launchTime = Instant::now(); // right when the binary is loaded
static Instant g_ccApplicationRunTime{};
static std::optional<asp::ThreadPool> s_loadThreadPool{};

struct PreLoadStageData {
    std::optional<asp::Thread<std::vector<AsyncImageLoadRequest>>> thread;
    std::unique_ptr<asp::Channel<AsyncImageLoadRequest>> channel;

    void cleanup() {
        thread.reset();
        channel.reset();
    }
};

struct GameLoadStageData {
    std::vector<AsyncImageLoadRequest> requests;
    std::unique_ptr<asp::Channel<AsyncImageLoadRequest*>> channel;

    void cleanup() {
        requests.clear();
        channel.reset();
    }
};

static PreLoadStageData g_preLoadStage;
static GameLoadStageData g_gameLoadStage;

template <bool SkipLoadImage = false>
static void asyncLoadLoadingLayerResources(std::vector<AsyncImageLoadRequest>&& resources) {
    asp::Thread<std::vector<AsyncImageLoadRequest>> thread;
    thread.setLoopFunction([](std::vector<AsyncImageLoadRequest>& items, auto& stopToken) {
        for (auto& item : items) {
            Result<> res = Ok();

            if constexpr (!SkipLoadImage) {
                res = item.loadImage();
                if (!res) {
                    log::warn("Error loading {}: {}", item.pngFile ? item.pngFile : "<null>", res.unwrapErr());
                    return;
                }
            }

            res = item.initImage();
            if (!res) {
                log::warn("Error loading {}: {}", item.pngFile ? item.pngFile : "<null>", res.unwrapErr());
            } else {
                g_preLoadStage.channel->push(std::move(item));
            }
        }

        stopToken.stop();
    });
    g_preLoadStage.thread.emplace(std::move(thread));
    g_preLoadStage.channel = std::make_unique<asp::Channel<AsyncImageLoadRequest>>();
    g_preLoadStage.thread->start(std::move(resources));
}

template <bool SkipLoadImage = false>
static void asyncLoadGameResources(std::vector<AsyncImageLoadRequest>&& resources) {
    g_gameLoadStage.requests = std::move(resources);
    g_gameLoadStage.channel = std::make_unique<asp::Channel<AsyncImageLoadRequest*>>();

    for (auto& img : g_gameLoadStage.requests) {
        s_loadThreadPool->pushTask([&img] {
            Result<> res = Ok();

            if constexpr (!SkipLoadImage) {
                res = img.loadImage();
                if (!res) {
                    log::warn("Error loading {}: {}", img.pngFile, res.unwrapErr());
                    return;
                }
            }

            res = img.initImage();
            if (!res) {
                log::warn("Error loading {}: {}", img.pngFile, res.unwrapErr());
            } else {
                g_gameLoadStage.channel->push(&img);
            }
        });
    }
}

class $modify(MyLoadingLayer, LoadingLayer) {
    struct Fields {
        bool finishedLoading = false;
        Instant startedLoadingGame;
        Instant finishedLoadingGame;
        Instant startedLoadingAssets;
    };

    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(LoadingLayer::loadAssets); // so we are only invoked once geode calls us
        BLAZE_HOOK_VERY_LAST(LoadingLayer::init); // idk geode was crashin without this
    }

    bool init(bool fromReload) {
        m_fields->startedLoadingGame = Instant::now();

        BLAZE_TIMER_START("(LoadingLayer::init) Initial setup");

        if (!CCLayer::init()) return false;

        if (fromReload) {
            // Init threadpool
            s_loadThreadPool.emplace(asp::ThreadPool{});
        }

        this->m_fromRefresh = fromReload;
        CCDirector::get()->m_bDisplayStats = true;

        if (fromReload) {
            // Load loadinglayer assets
            asyncLoadLoadingLayerResources(getLoadingLayerResources());
        }

        // FMOD is setup here on android
#ifdef GEODE_IS_ANDROID
        if (!fromReload) {
            FMODAudioEngine::get()->setup();
        }
#endif

        if (gm->m_switchModes) {
            gm->m_switchModes = false;
            GameLevelManager::get()->getLevelSaveData();
        }

        // Initialize textures
        BLAZE_TIMER_STEP("Main thread tasks");

        auto* sfcache = CCSpriteFrameCache::get();

        while (true) {
            if (g_preLoadStage.channel->empty()) {
                if (!g_preLoadStage.thread->isStopped()) {
                    std::this_thread::yield();
                    continue;
                } else if (g_preLoadStage.channel->empty()) {
                    break;
                }
            }

            auto iTask = g_preLoadStage.channel->popNow();
            auto res = iTask.initTexture();
            if (!res) {
                log::warn("Failed to init texture for {}: {}", iTask.pngFile, res.unwrapErr());
                continue;
            }

            iTask.addSpriteFrames((TextureQuality) gm->m_texQuality);
        }

        BLAZE_TIMER_STEP("LoadingLayer UI");

        this->addLoadingLayerUi();

        auto* acm = CCDirector::get()->getActionManager();
        auto action = CCSequence::create(
            CCDelayTime::create(0.f),
            CCCallFunc::create(this, callfunc_selector(MyLoadingLayer::customLoadStep)),
            nullptr
        );

        acm->addAction(action, this, false);

        m_fields->finishedLoadingGame = Instant::now();

        BLAZE_TIMER_END();

        return true;
    }

    void customLoadStep() {
        ZoneScoped;

        BLAZE_TIMER_START("(customLoadStep) Task queueing");

        m_fields->startedLoadingAssets = Instant::now();

        auto tcache = CCTextureCache::get();
        auto sfcache = CCSpriteFrameCache::get();

        // If it's a refresh, queue all resources to be loaded

        if (m_fromRefresh) {
            auto resources = getGameResources();
            asyncLoadGameResources(std::move(resources));
        }

        s_loadThreadPool->pushTask([] {
            ObjectToolbox::sharedState();
        });

        BLAZE_TIMER_STEP("ObjectManager::setup");

        ObjectManager::instance()->setup();

        BLAZE_TIMER_STEP("Effects & animations");

        auto afcache = CCAnimateFrameCache::sharedSpriteFrameCache();
        auto cmanager = CCContentManager::sharedManager();

        // @geode-ignore(unknown-resource)
        afcache->addSpriteFramesWithFile("Robot_AnimDesc.plist");

        cmanager->addDict("glassDestroy01.png", false);
        cmanager->addDict("coinPickupEffect.png", false);
        cmanager->addDict("explodeEffect.png", false);

        BLAZE_TIMER_STEP("Misc");

        AchievementManager::sharedState();

        // kinda obsolete lol
        if (gm->m_recordGameplay && !m_fromRefresh) {
            gm->m_everyPlaySetup = true;
        }

        // 13
        // lol?
        auto wave = CCCircleWave::create(10.f, 5.f, 0.3f, true);
        this->addChild(wave);
        wave->setPosition({-100.f, -100.f});

        BLAZE_TIMER_STEP("Main thread tasks");

        while (true) {
            if (g_gameLoadStage.channel->empty()) {
                if (s_loadThreadPool->isDoingWork()) {
                    std::this_thread::yield();
                    continue;
                } else if (g_gameLoadStage.channel->empty()) {
                    break;
                }
            }

            auto iTask = g_gameLoadStage.channel->popNow();
            auto res = iTask->initTexture();
            if (!res) {
                log::warn("Failed to init texture for {}: {}", iTask->pngFile, res.unwrapErr());
                continue;
            }

            if (iTask->plistFile) {
                s_loadThreadPool->pushTask([iTask] {
                    iTask->addSpriteFrames((TextureQuality) gm->m_texQuality);
                });
            }
        }

        BLAZE_TIMER_STEP("Wait for sprite frames");

        // also ensure fmod is initialized
        auto fae = HookedFMODAudioEngine::get();
        {
            std::lock_guard lock(fae->m_fields->initMutex);
        }

        s_loadThreadPool->join();

        BLAZE_TIMER_STEP("Final cleanup");

        CCTextInputNode::create(200.f, 50.f, "Temp", "Thonburi", 0x18, "bigFont.fnt");

        // cleanup in another thread because it can block for a few ms
        std::thread([] {
            g_preLoadStage.cleanup();
            g_gameLoadStage.cleanup();
            s_loadThreadPool.reset();
        }).detach();

        BLAZE_TIMER_END();

        this->finishLoading();
    }

    $override
    void loadAssets() {
        if (m_fields->finishedLoading) {
            m_loadStep = 14;
            LoadingLayer::loadAssets();
        } else {
            this->customLoadStep();
        }
    }

    void finishLoading() {
        auto finishTime = Instant::now();

        Duration tookTimeFull;
        if (m_fromRefresh) {
            tookTimeFull = finishTime.durationSince(m_fields->startedLoadingGame);
        } else {
            tookTimeFull = finishTime.durationSince(g_launchTime);
        }

        log::info("{} took {}, handing off..", m_fromRefresh ? "Reloading" : "Loading", tookTimeFull.toString());

#ifdef BLAZE_DEBUG
        if (!m_fromRefresh) {
            log::debug("- Geode entry remainder: {}", g_ccApplicationRunTime.durationSince(g_launchTime).toString());
            log::debug("- Initial game setup: {}", m_fields->startedLoadingGame.durationSince(g_ccApplicationRunTime).toString());
        }

        log::debug("- Pre-loading: {}", m_fields->finishedLoadingGame.durationSince(m_fields->startedLoadingGame).toString());
        log::debug("- Delay before asset loading: {}", m_fields->startedLoadingAssets.durationSince(m_fields->finishedLoadingGame).toString());
        log::debug("- Asset loading: {}", finishTime.durationSince(m_fields->startedLoadingAssets).toString());
#endif

        m_fields->finishedLoading = true;
        this->loadAssets();
    }

    // Boring stuff
    void addLoadingLayerUi() {
        auto winSize = CCDirector::get()->getWinSize();

        auto bg = CCSprite::create("game_bg_01_001.png");
        this->addChild(bg);
        bg->setPosition(winSize / 2.f);

        CCApplication::get(); // ??
        bg->setScale(CCDirector::get()->getScreenScaleFactorMax());
        bg->setColor({0x00, 0x66, 0xff});

        auto logo = CCSprite::createWithSpriteFrameName("GJ_logo_001.png");
        this->addChild(logo);
        logo->setPosition(winSize / 2.f);

        auto robtopLogo = CCSprite::createWithSpriteFrameName("RobTopLogoBig_001.png");
        this->addChild(robtopLogo);
        robtopLogo->setPosition(logo->getPosition() + CCPoint{0.f, 80.f});

        m_unknown2 = true;
        m_loadStep = 0;

        auto cocosLogo = CCSprite::createWithSpriteFrameName("cocos2DxLogo.png");
        this->addChild(cocosLogo);
        cocosLogo->setPosition({winSize.width - 34.f, 13.f});
        cocosLogo->setScale(0.6f);

        auto fmodLogo = CCSprite::createWithSpriteFrameName("fmodLogo.png");
        this->addChild(fmodLogo);
        fmodLogo->setPosition(cocosLogo->getPosition() + CCPoint{0.f, 20.f});
        fmodLogo->setScale(0.6f);
        fmodLogo->setColor({0, 0, 0});

        m_caption = CCLabelBMFont::create(this->getLoadingString(), "goldFont.fnt");
        this->addChild(m_caption);
        m_caption->setPosition({winSize.width * 0.5f, winSize.height * 0.5f - 70.f});
        m_caption->setScale(0.7f);
        m_caption->setVisible(false);

        auto lstr = this->getLoadingString();
        this->m_textArea =  TextArea::create(lstr, "goldFont.fnt", 1.f, 440.f, {0.5f, 0.5f}, 28.f, true);
        this->addChild(m_textArea);
        m_textArea->setPosition({winSize.width / 2.f, winSize.height / 2.f - 100.f});
        m_textArea->setScale(0.7f);

        auto csize = m_caption->getContentSize();

        if (csize.width > 300.f) {
            m_caption->setScale(300.f / csize.width);
        }

        float scale = std::min(m_caption->getScale(), 0.7f);
        m_caption->setScale(scale);

        auto groove = CCSprite::create("slidergroove.png");
        this->addChild(groove, 3);

        m_sliderBar = CCSprite::create("sliderBar.png");
        m_sliderGrooveXPos = groove->getTextureRect().size.width - 4.f;
        m_sliderGrooveHeight = 8.f;

        ccTexParams params = {
            GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT
        };
        m_sliderBar->getTexture()->setTexParameters(&params);
        groove->addChild(m_sliderBar, -1);
        m_sliderBar->setAnchorPoint({0.f, 0.f});
        m_sliderBar->setPosition({2.f, 4.f});
        auto txareaPos = m_textArea->getPosition();
        groove->setPosition({m_caption->getPosition().x, txareaPos.y + 40.f});

        // TODO
#ifndef GEODE_IS_MACOS
        this->updateProgress(0);
#else
        m_sliderBar->setScaleX(0.f);
#endif
    }
};

void blaze::startPreInit() {
    g_ccApplicationRunTime = Instant::now();

    // Check if our compile-time crc32 algorithm works correctly (should never fail, but we'll keep as a sanity check)
    if (blaze::hashStringRuntime("hai uwu") != BLAZE_STRING_HASH("hai uwu")) {
        log::error("ERROR: blaze detected abnormality in the hashing algorithm.");
        log::error("Mismatch between runtime and compile-time hashing algorithms was detected,");
        log::error("- Value computed at compile time: 0x{:08x}", BLAZE_STRING_HASH("hai uwu"));
        log::error("- Value computed at runtime: 0x{:08x}", blaze::hashStringRuntime("hai uwu"));
        log::error("");
        log::error("This should *never ever* happen, the game will exit now.");
        std::abort();
    }

    BLAZE_TIMER_START("CCApplication::run (managers pre-setup)");

    // early init glfw
#ifdef GEODE_IS_WINDOWS
    std::optional<asp::Thread<>> glfwInitThread;
    if (blaze::settings().asyncGlfw && g_canHookGlfw) {
        glfwInitThread.emplace(asp::Thread<>{});
        glfwInitThread->setLoopFunction([](auto& stopToken) {
            blaze::customGlfwInit();
            stopToken.stop();
        });
        glfwInitThread->start();
    }
#endif

    CCFileUtils::get()->addSearchPath("Resources");

    BLAZE_TIMER_STEP("GameManager::init");

    // initialize gamemanager, we have to do it on main thread right here
    // TODO: do we really?
    gm = GameManager::get();

    // setup fmod asynchronously
    #ifndef GEODE_IS_ANDROID
    FMODAudioEngine::get()->setup();
    #endif

    // initialize llm in another thread.
    // we can only do this if we carefully checked that all the functions afterwards are thread-safe and do not call autorelease
    asp::Thread<> llmLoadThread;
    llmLoadThread.setLoopFunction([](auto& stopToken) {
        LocalLevelManager::get();
        stopToken.stop();
    });
    llmLoadThread.start();

    BLAZE_TIMER_STEP("Preparation for asset preloading");

    // set appropriate texture quality
    auto tq = gm->m_texQuality;
    CCDirector::get()->updateContentScale((TextureQuality)tq);
    CCTexture2D::setDefaultAlphaPixelFormat(cocos2d::kCCTexture2DPixelFormat_Default);

    // Init threadpool
    s_loadThreadPool.emplace(asp::ThreadPool{});

    auto resources1 = getLoadingLayerResources();
    auto resources2 = getGameResources();

    // Load images

    for (auto& img : resources1) {
        s_loadThreadPool->pushTask([&img] {
            auto res = img.loadImage();
            if (!res) {
                log::warn("Failed to initialize image: {}", res.unwrapErr());
            }
        });
    }

    for (auto& img : resources2) {
        s_loadThreadPool->pushTask([&img] {
            auto res = img.loadImage();
            if (!res) {
                log::warn("Failed to initialize image: {}", res.unwrapErr());
            }
        });
    }

    // wait for all images to be preloaded (should be pretty quick, they are not decoded yet)
    s_loadThreadPool->join();

    // start decoding the images in background
    asyncLoadLoadingLayerResources<true>(std::move(resources1));

    // rest of the images (for the game itself)
    asyncLoadGameResources<true>(std::move(resources2));

    CCFileUtils::get()->removeSearchPath("Resources");

    // wait until llm finishes initialization
    BLAZE_TIMER_STEP("Wait for LLM init job to finish");
    llmLoadThread.join();

#ifdef GEODE_IS_WINDOWS
    if (glfwInitThread) {
        BLAZE_TIMER_STEP("Wait for async GLFW job to finish");
        glfwInitThread->join();
    }
#endif

    BLAZE_TIMER_END();
}

#ifdef GEODE_IS_WINDOWS
class $modify(CCApplication) {
    int run() {
        blaze::startPreInit();

        // finally go back to running the rest of the game
        return CCApplication::run();
    }
};
#elif defined(GEODE_IS_ANDROID)
class $modify(AppDelegate) {
    bool applicationDidFinishLaunching() {
        blaze::startPreInit();

        // finally go back to running the rest of the game
        return AppDelegate::applicationDidFinishLaunching();
    }
};
#endif
// Macos entry is defined in load.mm
