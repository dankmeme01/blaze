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

AsyncLoad::TaskHandle LoadModule::loadSheet(ZStringView name) {
    auto& alm = ALManager::get();
    return alm.loadSpritesheet(name, [this, name = std::string{name}, start = asp::Instant::now()](auto result) {
        if (!result) {
            log::error("Failed to load spritesheet {}: {}", name, result.unwrapErr());
            return;
        }

        log::debug("Loaded spritesheet {} in {}", name, start.elapsed());
    });
}

void LoadModule::loadSheet(AsyncLoad::TaskGroup& group, ZStringView name) {
    group.add(this->loadSheet(name));
}

AsyncLoad::TaskHandle LoadModule::loadImage(ZStringView name) {
    auto& alm = ALManager::get();
    auto handle = alm.loadTextureEager(name, [this, name = std::string{name}, start = asp::Instant::now()](Result<Ref<CCTexture2D>> result) {
        if (!result) {
            log::error("Failed to load image {}: {}", name, result.unwrapErr());
            return;
        }

        log::debug("Loaded image {} in {}", name, start.elapsed());
    });
    handle.setName(fmt::format("Blaze Load {}", name));
    return handle;
}

void LoadModule::loadImage(AsyncLoad::TaskGroup& group, ZStringView name) {
    // loadImage uses loadTextureEager, that may complete immediately and return a null task handle
    auto h = this->loadImage(name);
    if (h) group.add(h);
}

AsyncLoad::TaskHandle LoadModule::loadFont(ZStringView name) {
    auto view = name.view();
    if (view.ends_with(".fnt")) {
        view.remove_suffix(4);
    }

    auto handle = this->loadImage(fmt::format("{}.png", view));

    // load font
    auto start = asp::Instant::now();
    FNTConfigLoadFile(fmt::format("{}.fnt", name).c_str());
    log::debug("Loaded font {} in {}", name, start.elapsed());

    return handle;
}

void LoadModule::loadFont(AsyncLoad::TaskGroup& group, ZStringView name) {
    // loadFont calls loadImage which uses loadTextureEager, that may complete immediately and return a null task handle
    auto h = this->loadFont(name);
    if (h) group.add(h);
}

void LoadModule::onLoadingLayerInit(bool reload) {
    // search paths have been initialized, this is ok now
    arc::spawnBlocking<void>([&] {
        this->populateFpffCache();
    });

    // now load the resources required for loading layer
    TaskGroup group;
    this->loadSheet(group, "GJ_LaunchSheet");
    this->loadImage(group, "game_bg_01_001.png");
    this->loadImage(group, "slidergroove.png");
    this->loadImage(group, "sliderBar.png");
    this->loadFont(group, "goldFont");

    group.wait();
}

void LoadModule::onLoadingLayerInitPost() {
    // disable vsync so frame transitions are fast during loadinglayer
    this->disableVsync();
}

TaskGroup LoadModule::onLoadStart() {
    m_loadStartTime = asp::Instant::now();

    TaskGroup group;
    this->loadSheet(group, "GJ_GameSheet");
    this->loadSheet(group, "GJ_GameSheet02");
    this->loadSheet(group, "GJ_GameSheet03");
    this->loadSheet(group, "GJ_GameSheetEditor");
    this->loadSheet(group, "GJ_GameSheet04");
    this->loadSheet(group, "GJ_GameSheetGlow");
    this->loadSheet(group, "FireSheet_01");
    this->loadSheet(group, "GJ_ShopSheet");
    this->loadSheet(group, "GJ_ParticleSheet");
    this->loadSheet(group, "PixelSheet_01");

    this->loadImage(group, "smallDot.png");
    this->loadImage(group, "square02_001.png");
    this->loadSheet(group, "CCControlColourPickerSpriteSheet");

    this->loadImage(group, "GJ_gradientBG.png");
    this->loadImage(group, "edit_barBG_001.png");
    this->loadImage(group, "GJ_button_01.png");
    this->loadImage(group, "slidergroove2.png");

    this->loadImage(group, "GJ_square01.png");
    this->loadImage(group, "GJ_square02.png");
    this->loadImage(group, "GJ_square03.png");
    this->loadImage(group, "GJ_square04.png");
    this->loadImage(group, "GJ_square05.png");
    this->loadImage(group, "gravityLine_001.png");

    this->loadFont(group, "bigFont");
    this->loadFont(group, "chatFont");

    this->loadSheet(group, "Robot_AnimDesc");

    // GD doesn't load the ones below on LoadingLayer, but they are quickly used in MenuLayer
    this->loadImage(group, "groundSquare_01_001.png");
    this->loadImage(group, "square.png");

    group.close();

    return group;
}

void LoadModule::onLoadFinished() {
    m_loadFinishTime = asp::Instant::now();
    this->restoreVsync();
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

    TaskGroup group;

    for (auto mod : mods) {
        if (!mod->isOrWillBeEnabled()) continue;

        auto searchPathRoot = dirs::getModRuntimeDir() / mod->getID() / "resources";

        // geode.loader resource is stored somewhere else, which is already added anyway
        if (!mod->isInternal()) {
            CCFileUtils::get()->addSearchPath(searchPathRoot.c_str());
        }

        // only thing needs previous setup is spritesheets
        auto& sheets = mod->getMetadata().getSpritesheets();
        if (sheets.empty()) continue;

        // quick existence check to skip processing the mod
        // this may not exist if the mod hadn't been unzipped (e.g. outdated gd version)
        if (!mod->isInternal() && !searchPathRoot.exists()) {
            log::debug("Skipping mod {}, no resource directory", mod->getID());
            continue;
        }

        log::debug("{}", mod->getID());
        log::NestScope nest;

        for (auto& sheet : sheets) {
            log::debug("Enqueue sheet {}", sheet);

            group.add(ALManager::get().loadSpritesheet(sheet, [&, start = asp::Instant::now()](auto result) {
                if (!result) {
                    log::error("Failed to load mod spritesheet {}: {}", sheet, result.unwrapErr());
                    return;
                }

                log::debug("Loaded spritesheet {} in {}", sheet, start.elapsed());
            }));
        }
    }

    group.wait();

    log::info("Took {} to load all mod resources ({} mods, {} sheets)", start.elapsed(), mods.size(), group.totalTasks());
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
        if (!AsyncLoad::fileExists(result)) {
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
        LoadModule::get().loadModResourcesBlocking(std::move(mods));
        return ListenerResult::Stop;
    }, Priority::Stub).leak();
}

}
