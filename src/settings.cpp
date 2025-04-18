#include "settings.hpp"

#include <Geode/loader/Setting.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

namespace blaze {
    _settings& settings() {
        static _settings settings;

        if (!settings._init) {
            settings._init = true;
            settings.fastDecompression = Mod::get()->getSettingValue<bool>("faster-decompression");
            settings.imageCache = Mod::get()->getSettingValue<bool>("image-cache");
            settings.imageCacheSmall = Mod::get()->getSettingValue<bool>("image-cache-small");
            settings.asyncGlfw = Mod::get()->getSettingValue<bool>("async-glfw");
            settings.asyncFmod = Mod::get()->getSettingValue<bool>("async-fmod");
            settings.fastSaving = Mod::get()->getSettingValue<bool>("fast-saving");
            settings.uncompressedSaves = Mod::get()->getSettingValue<bool>("uncompressed-saves");
            settings.lowMemory = Mod::get()->getSettingValue<bool>("low-memory-mode");
            settings.loadMore = Mod::get()->getSettingValue<bool>("load-more");
        }

        return settings;
    }
}

#define s_listen(id, name) listenForSettingChanges(id, +[](bool _v) { blaze::settings().name = _v; })

$execute {
    s_listen("image-cache", imageCache);
    s_listen("image-cache-small", imageCacheSmall);
    s_listen("fast-saving", fastSaving);
    s_listen("uncompressed-saves", uncompressedSaves);
    s_listen("low-memory-mode", lowMemory);
}
