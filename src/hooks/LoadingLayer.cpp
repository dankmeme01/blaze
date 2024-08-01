// Blaze LoadingLayer hooks.
//
// Completely rewrites `LoadingLayer::loadAssets` to be multi-threaded.

#include <Geode/Geode.hpp>

#include <asp/sync/Mutex.hpp>
#include <asp/thread/ThreadPool.hpp>
#include <manager.hpp>
#include <ccimageext.hpp>
#include <tracing.hpp>

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

struct MTTextureInitTask {
    std::string sheetName;
    CCImage* img;
    const char* plistToLoad = nullptr;
    std::string plistFullPathToLoad;
};

static asp::Mutex<> texCacheMutex;
static asp::Mutex<> sfcacheMutex;

static std::optional<MTTextureInitTask> asyncLoadImage(const char* sheet, const char* plistToLoad) {
    ZoneScoped;

    auto pathKey = CCFileUtils::get()->fullPathForFilename(sheet, false);
    if (pathKey.empty()) {
        log::warn("Failed to find: {}", sheet);
        return std::nullopt;
    }

    auto _l = texCacheMutex.lock();
    if (CCTextureCache::get()->m_pTextures->objectForKey(pathKey)) {
        log::warn("Already loaded: {}", pathKey);
        return std::nullopt;
    }
    _l.unlock();

    size_t dataSize;
    auto data = LoadManager::get().readFile(pathKey.c_str(), dataSize);

    if (!data || dataSize == 0) {
        log::warn("Failed to open: {}", pathKey);
        return std::nullopt;
    }

    auto img = new CCImage();
    auto ret = static_cast<blaze::CCImageExt*>(img)->initWithSPNGOrCache(data.get(), dataSize, pathKey.c_str());

    if (!ret) {
        img->release();
        log::error("Failed to load image {}: {}", sheet, ret.unwrapErr());
        return std::nullopt;
    }

    std::string_view pksv(pathKey);

    return MTTextureInitTask {
        .sheetName = std::move(pathKey),
        .img = img,
        .plistToLoad = plistToLoad,
        // oh the horrors
        .plistFullPathToLoad = plistToLoad ? (std::string(pksv.substr(0, pksv.find(".png"))) + ".plist") : ""
    };
}

static void asyncAddSpriteFrames(const char* fullPlistGuess, const char* plist, CCTexture2D* texture, const std::string& sheetName) {
    ZoneScoped;

    CCDictionary* dict;

    {
        ZoneScopedN("asyndAddSpriteFrames ccdict");

        GEODE_ANDROID(auto _zlck = LoadManager::get().zipFileMutex.lock());
        dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistGuess);

        if (!dict) {
            dict = CCDictionary::createWithContentsOfFileThreadSafe(CCFileUtils::get()->fullPathForFilename(plist, false).c_str());
        }

        GEODE_ANDROID(_zlck.unlock());

        if (!dict) {
            log::warn("failed to find the plist for {}", plist);
            auto _mtx = texCacheMutex.lock();
            CCTextureCache::get()->m_pTextures->removeObjectForKey(sheetName);
            return;
        }
    }

    {
        ZoneScopedN("asyncAddSpriteFrames addSpriteFrames");
        auto _lastlck = sfcacheMutex.lock();
        _addSpriteFramesWithDictionary(dict, texture);
        _lastlck.unlock();
    }

    dict->release();
}

static void loadFont(const char* name) {
    ZoneScoped;

    CCLabelBMFont::create(" ", name);
}

#include <Geode/modify/LoadingLayer.hpp>
class $modify(MyLoadingLayer, LoadingLayer) {
    struct Fields {
        asp::ThreadPool threadPool{20};
        bool finishedLoading = false;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("LoadingLayer::loadAssets", 999999999).unwrap(); // so we are only invoked once geode calls us
        BLAZE_HOOK_VERY_LAST(LoadingLayer::init); // idk geode was crashin without this
    }

    // this will eventually get called by geode
    $override
    void loadAssets() {
        if (m_fields->finishedLoading) {
            m_loadStep = 14;
            LoadingLayer::loadAssets();
        } else {
            this->customLoadStep();
        }
    }

    void customLoadStep() {
        ZoneScoped;

        auto startTime = std::chrono::high_resolution_clock::now();

        auto tcache = CCTextureCache::get();
        auto sfcache = CCSpriteFrameCache::get();

        struct LoadSheetTask {
            const char *sheet, *plist;
        };

        struct LoadImageTask {
            const char *image;
        };

        struct LoadCustomTask {
            std::function<void()> func;
        };

        using PreloadTask = std::variant<LoadSheetTask, LoadImageTask, LoadCustomTask>;

        using MainThreadTask = std::variant<MTTextureInitTask, LoadCustomTask>;

        std::vector<PreloadTask> tasks;
        std::vector<MainThreadTask> mtTasks;

#define MAKE_LOADSHEET(name) tasks.push_back(LoadSheetTask { .sheet = name".png", .plist = name".plist" })
#define MAKE_LOADIMG(name) tasks.push_back(LoadImageTask { .image = name".png" })
#define MAKE_LOADCUSTOM(ft) tasks.push_back(LoadCustomTask { .func = [&] ft })

        MAKE_LOADSHEET("GJ_GameSheet");
        MAKE_LOADSHEET("GJ_GameSheet02");
        MAKE_LOADSHEET("GJ_GameSheet03");
        MAKE_LOADSHEET("GJ_GameSheetEditor");
        MAKE_LOADSHEET("GJ_GameSheet04");
        MAKE_LOADSHEET("GJ_GameSheetGlow");

        MAKE_LOADSHEET("FireSheet_01");
        MAKE_LOADSHEET("GJ_ShopSheet");
        MAKE_LOADIMG("smallDot");
        MAKE_LOADIMG("square02_001");
        MAKE_LOADSHEET("GJ_ParticleSheet");
        MAKE_LOADSHEET("PixelSheet_01");

        MAKE_LOADSHEET("CCControlColourPickerSpriteSheet");
        MAKE_LOADIMG("GJ_gradientBG");
        MAKE_LOADIMG("edit_barBG_001");
        MAKE_LOADIMG("GJ_button_01");
        MAKE_LOADIMG("slidergroove2");

        MAKE_LOADIMG("GJ_square01");
        MAKE_LOADIMG("GJ_square02");
        MAKE_LOADIMG("GJ_square03");
        MAKE_LOADIMG("GJ_square04");
        MAKE_LOADIMG("GJ_square05");
        MAKE_LOADIMG("gravityLine_001");

        MAKE_LOADCUSTOM({
            ZoneScopedN("ObjectToolbox::sharedState");

            ObjectToolbox::sharedState();
        });

        loadFont("chatFont.fnt");
        loadFont("bigFont.fnt");
        // loadFont("goldFont.fnt");

        ObjectManager::instance()->setup();

        auto afcache = CCAnimateFrameCache::sharedSpriteFrameCache();
        auto cmanager = CCContentManager::sharedManager();

        afcache->addSpriteFramesWithFile("Robot_AnimDesc.plist");

        cmanager->addDict("glassDestroy01.png", false);
        cmanager->addDict("coinPickupEffect.png", false);
        cmanager->addDict("explodeEffect.png", false);

        AchievementManager::sharedState();

        // kinda obsolete lol
        auto* gm = GameManager::get();
        if (gm->m_recordGameplay && !m_fromRefresh) {
            gm->m_everyPlaySetup = true;
        }

        // might be wrong, might be Thonburi
        CCTextInputNode::create(200.f, 50.f, "Temp", "bigFont.fnt");

        // 13
        // lol?
        auto wave = CCCircleWave::create(10.f, 5.f, 0.3f, true);
        this->addChild(wave);
        wave->setPosition({-100.f, -100.f});

        static asp::Channel<MainThreadTask> mainThreadQueue;

        for (auto& task : tasks) {
            m_fields->threadPool.pushTask([&, task = std::move(task)] {
                // do stuff..
                std::visit(makeVisitor {
                    [&](const LoadSheetTask& task) {
                        if (auto res = asyncLoadImage(task.sheet, task.plist)) {
                            mainThreadQueue.push(std::move(res.value()));
                        }
                    },
                    [&](const LoadImageTask& task) {
                        if (auto res = asyncLoadImage(task.image, nullptr)) {
                            mainThreadQueue.push(std::move(res.value()));
                        }
                    },
                    [&](const LoadCustomTask& task) {
                        task.func();
                    },
                }, task);
            });
        }

        while (true) {
            ZoneScopedN("tasks");

            if (mainThreadQueue.empty()) {
                if (m_fields->threadPool.isDoingWork()) {
                    std::this_thread::yield();
                    continue;
                } else if (mainThreadQueue.empty()) {
                    break;
                }
            }

            auto thing = mainThreadQueue.popNow();
            std::visit(makeVisitor {
                [&](const MTTextureInitTask& task) {
                    ZoneScopedN("texture init task");

                    auto texture = new CCTexture2D();
                    if (!texture->initWithImage(task.img)) {
                        delete texture;
                        task.img->release();
                        log::warn("failed to init cctexture2d: {}", task.img);
                        return;
                    }

                    auto _lck = texCacheMutex.lock();
                    tcache->m_pTextures->setObject(texture, task.sheetName);
                    _lck.unlock();

                    texture->release();
                    task.img->release();

                    if (task.plistToLoad) {
                        // adding sprite frames
                        m_fields->threadPool.pushTask([
                            texture,
                            plist = task.plistToLoad,
                            fullPlist = task.plistFullPathToLoad,
                            sheetName = task.sheetName
                        ] {
                            asyncAddSpriteFrames(fullPlist.c_str(), plist, texture, sheetName);
                        });
                    }
                },
                [&](const LoadCustomTask& task) {
                    task.func();
                }
            }, thing);
        }

        m_fields->threadPool.join();

        auto tookTime = std::chrono::high_resolution_clock::now() - startTime;
        log::debug("Loading took {}, handing off..", formatDuration(tookTime));

        this->finishLoading();

        // run deferred tasks..
    }

    void finishLoading() {
        m_fields->finishedLoading = true;
        this->loadAssets();
    }

    // init reimpl
    bool init(bool fromReload) {
        if (!CCLayer::init()) return false;

        auto time1 = benchTimer();

        this->m_fromRefresh = fromReload;
        CCDirector::get()->m_bDisplayStats = true;
        CCTexture2D::setDefaultAlphaPixelFormat(cocos2d::kCCTexture2DPixelFormat_Default);

        if (!fromReload) {
            FMODAudioEngine::get()->setup();
        }

        auto time2 = benchTimer();

        auto* gm = GameManager::get();
        if (gm->m_switchModes) {
            gm->m_switchModes = false;
            GameLevelManager::get()->getLevelSaveData();
        }

        auto time3 = benchTimer();

        auto* tcache = CCTextureCache::get();
        tcache->addImage("GJ_LaunchSheet.png", false);
        auto* sfcache = CCSpriteFrameCache::get();
        sfcache->addSpriteFramesWithFile("GJ_LaunchSheet.plist");

        auto time4 = benchTimer();

        auto winSize = CCDirector::get()->getWinSize();

        gm->loadBackground(1);

        auto time5 = benchTimer();

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

        // xmm0-3 = 28.f, 0.5f, 1.f, 440.f
        // why are they in reverse?? idk
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

        this->updateProgress(0);
        auto* acm = CCDirector::get()->getActionManager();
        auto action = CCSequence::create(
            CCDelayTime::create(0.f),
            CCCallFunc::create(this, callfunc_selector(LoadingLayer::loadAssets)),
            nullptr
        );

        acm->addAction(action, this, false);

        auto time6 = benchTimer();
        // log::debug("Period 1: {}", formatDuration(time2 - time1));
        // log::debug("Period 2: {}", formatDuration(time3 - time2));
        // log::debug("Period 3: {}", formatDuration(time4 - time3));
        // log::debug("Period 4: {}", formatDuration(time5 - time4));
        // log::debug("Period 5: {}", formatDuration(time6 - time5));

        return true;
    }
};