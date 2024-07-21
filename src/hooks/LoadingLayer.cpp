#include <Geode/Geode.hpp>

#include <asp/sync/Mutex.hpp>
#include <asp/thread/ThreadPool.hpp>
#include <manager.hpp>
#include <ccimageext.hpp>

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
    auto ret = static_cast<blaze::CCImageExt*>(img)->initWithSPNGOrCache(data, dataSize, pathKey.c_str());
    delete[] data;

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
    GEODE_ANDROID(auto _zlck = LoadManager::get().zipFileMutex.lock());
    CCDictionary* dict = CCDictionary::createWithContentsOfFileThreadSafe(fullPlistGuess);

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

    auto _lastlck = sfcacheMutex.lock();
    _addSpriteFramesWithDictionary(dict, texture);
    _lastlck.unlock();

    dict->release();
}

static void loadFont(const char* name) {
    CCLabelBMFont::create(" ", name);
}

#include <Geode/modify/LoadingLayer.hpp>
class $modify(MyLoadingLayer, LoadingLayer) {
    struct Fields {
        asp::ThreadPool threadPool{20};
        bool finishedLoading = false;
    };

    static void onModify(auto& self) {
        (void) self.setHookPriority("LoadingLayer::loadAssets", 999999999); // so we are only invoked once geode calls us
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
};