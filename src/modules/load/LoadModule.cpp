#include "LoadModule.hpp"
#include <Geode/modify/MenuLayer.hpp>
#include <AsyncLoad/FileUtils.hpp>
#include <AsyncLoad/SpriteFrames.hpp>
#include <AsyncLoad/Fonts.hpp>
#include <asp/fs.hpp>
#include <platform/platform.hpp>

using namespace geode::prelude;
using namespace AsyncLoad;

namespace blaze {

LoadModule::LoadModule() {}

void LoadModule::onEntry() {
    m_entryTime = asp::Instant::now();
    m_processStartTime = platform::getProcessStartTime();

    // TODO: module disabled, do nothing?
}

void LoadModule::loadSheet(ZStringView name) {
    m_awaitingTasks += 1;

    auto& alm = ALManager::get();
    alm.loadSpritesheet(name, [this, name = std::string{name}, start = asp::Instant::now()](Result<> result) {
        m_awaitingTasks--;

        if (!result) {
            log::error("Failed to load spritesheet {}: {}", name, result.unwrapErr());
            return;
        }

        log::debug("Loaded spritesheet {} in {}", name, start.elapsed());
    }).leak();
}

void LoadModule::loadImage(ZStringView name) {
    m_awaitingTasks += 1;

    auto& alm = ALManager::get();
    alm.loadTexture(name, [this, name = std::string{name}, start = asp::Instant::now()](Result<Ref<CCTexture2D>> result) {
        m_awaitingTasks--;

        if (!result) {
            log::error("Failed to load image {}: {}", name, result.unwrapErr());
            return;
        }

        log::debug("Loaded image {} in {}", name, start.elapsed());
    }).leak();
}

void LoadModule::loadFont(ZStringView name) {
    auto view = name.view();
    if (view.ends_with(".fnt")) {
        view.remove_suffix(4);
    }

    this->loadImage(fmt::format("{}.png", view));

    // load font
    auto start = asp::Instant::now();
    FNTConfigLoadFile(fmt::format("{}.fnt", name).c_str());
    log::debug("Loaded font {} in {}", name, start.elapsed());
}

void LoadModule::onLoadingLayerInit() {
    // search paths have been initialized, this is ok now
    arc::spawnBlocking<void>([&] {
        this->populateFpffCache();
    });

    // now load the resources required for loading layer
    this->loadSheet("GJ_LaunchSheet");
    this->loadImage("game_bg_01_001.png");
    this->loadImage("slidergroove.png");
    this->loadImage("sliderBar.png");
    this->loadFont("goldFont");

    while (m_awaitingTasks > 0) {
        ALManager::get().lendMainThread();
        std::this_thread::yield();
    }
}

void LoadModule::onLoadStart() {
    m_loadStartTime = asp::Instant::now();

    this->loadSheet("GJ_GameSheet");
    this->loadSheet("GJ_GameSheet02");
    this->loadSheet("GJ_GameSheet03");
    this->loadSheet("GJ_GameSheetEditor");
    this->loadSheet("GJ_GameSheet04");
    this->loadSheet("GJ_GameSheetGlow");
    this->loadSheet("FireSheet_01");
    this->loadSheet("GJ_ShopSheet");
    this->loadSheet("GJ_ParticleSheet");
    this->loadSheet("PixelSheet_01");

    this->loadImage("smallDot.png");
    this->loadImage("square02_001.png");
    this->loadSheet("CCControlColourPickerSpriteSheet");

    this->loadImage("GJ_gradientBG.png");
    this->loadImage("edit_barBG_001.png");
    this->loadImage("GJ_button_01.png");
    this->loadImage("slidergroove2.png");

    this->loadImage("GJ_square01.png");
    this->loadImage("GJ_square02.png");
    this->loadImage("GJ_square03.png");
    this->loadImage("GJ_square04.png");
    this->loadImage("GJ_square05.png");
    this->loadImage("gravityLine_001.png");

    this->loadFont("bigFont");
    this->loadFont("chatFont");

    // GD doesn't load the ones below on LoadingLayer, but they are quickly used in MenuLayer
    this->loadImage("groundSquare_01_001.png");
    this->loadImage("square.png");
}

void LoadModule::onLoadFinished() {
    m_loadFinishTime = asp::Instant::now();
}

void LoadModule::onMenuLayer() {
    m_menuLayerTime = asp::Instant::now();

    log::info("Game fully loaded in {} ({} excluding OS startup)!", m_processStartTime.elapsed(), m_entryTime.elapsed());
    log::debug("- Time before Geode entry: {}", m_entryTime.durationSince(m_processStartTime));
    log::debug("- Load start: {}", m_loadStartTime.durationSince(m_entryTime));
    log::debug("- Load finish: {}", m_loadFinishTime.durationSince(m_loadStartTime));
    log::debug("- Time to MenuLayer: {}", m_menuLayerTime.durationSince(m_loadFinishTime));
}

void LoadModule::loadModResourcesBlocking(std::vector<Mod*> mods) {
    auto start = asp::Instant::now();

    size_t queued = 0;
    size_t totalSheets = 0;

    for (auto mod : mods) {
        if (!mod->isOrWillBeEnabled()) continue;

        if (!mod->isInternal()) {
            // geode.loader resource is stored somewhere else, which is already added anyway
            auto searchPathRoot = dirs::getModRuntimeDir() / mod->getID() / "resources";
            CCFileUtils::get()->addSearchPath(utils::string::pathToString(searchPathRoot).c_str());
        }

        // only thing needs previous setup is spritesheets
        auto& sheets = mod->getMetadata().getSpritesheets();
        if (sheets.empty()) continue;

        log::debug("{}", mod->getID());
        log::NestScope nest;

        for (auto& sheet : sheets) {
            log::debug("Enqueue sheet {}", sheet);
            queued++;
            totalSheets++;

            ALManager::get().loadSpritesheet(sheet, [&, start = asp::Instant::now()](Result<> result) {
                queued--;

                if (!result) {
                    log::error("Failed to load spritesheet {}: {}", sheet, result.unwrapErr());
                    return;
                }

                log::debug("Loaded spritesheet {} in {}", sheet, start.elapsed());
            }).leak();
        }
    }

    while (queued > 0) {
        ALManager::get().lendMainThread();
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

    log::info("Took {} to load all mod resources ({} mods, {} sheets)", start.elapsed(), mods.size(), totalSheets);
}

size_t LoadModule::runningTasks() const {
    return m_awaitingTasks;
}

void LoadModule::populateFpffCache() {
    // these are files that are virtually always accessed somewhere during game load,
    // call fullPathForFilename so when they are requested, they are returned very quickly
    // we can allow ourselves to do this because asyncload's fpff is thread safe
    auto paths = std::array {
        "GJ_LaunchSheet.png",
        "GJ_LaunchSheet.plist",
        "game_bg_01_001.png",
        "slidergroove.png",
        "sliderBar.png",
        "goldFont.fnt",
        "goldFont.png",

        // LoadingLayer
        "GJ_GameSheet.png",
        "GJ_GameSheet.plist",
        "GJ_GameSheet02.png",
        "GJ_GameSheet02.plist",
        "GJ_GameSheet03.png",
        "GJ_GameSheet03.plist",
        "GJ_GameSheet04.png",
        "GJ_GameSheet04.plist",
        "GJ_GameSheetEditor.png",
        "GJ_GameSheetEditor.plist",
        "GJ_GameSheetGlow.png",
        "GJ_GameSheetGlow.plist",

        "FireSheet_01.png",
        "FireSheet_01.plist",
        "GJ_ShopSheet.png",
        "GJ_ShopSheet.plist",
        "smallDot.png",
        "square02_001.png",
        "GJ_ParticleSheet.png",
        "GJ_ParticleSheet.plist",
        "PixelSheet_01.png",
        "PixelSheet_01.plist",

        "CCControlColourPickerSpriteSheet.png",
        "CCControlColourPickerSpriteSheet.plist",
        "GJ_gradientBG.png",
        "edit_barBG_001.png",
        "GJ_button_01.png",
        "slidergroove2.png",

        "GJ_square01.png",
        "GJ_square02.png",
        "GJ_square03.png",
        "GJ_square04.png",
        "GJ_square05.png",
        "gravityLine_001.png",

        "bigFont.fnt",
        "bigFont.png",
        "chatFont.fnt",
        "chatFont.png",
    };

    for (auto path : paths) {
        auto result = AsyncLoad::fullPathForFilename(path);

#ifdef BLAZE_DEBUG
        if (!asp::fs::exists(result)) {
            log::warn("FPFF cache tried loading non-existing file: {}", result);
        }
#endif
    }

    log::debug("Pre-populated FPFF cache with {} entries", paths.size());
}

$on_game(Loaded) {
    LoadModule::get().onMenuLayer();
}

$on_mod(Loaded) {
    LoaderUpdateModResourcesEvent().listen([](std::vector<Mod*> mods) {
        log::debug("Receive blaze");
        LoadModule::get().loadModResourcesBlocking(std::move(mods));
        return ListenerResult::Stop;
    }, Priority::Stub).leak();
}

}
