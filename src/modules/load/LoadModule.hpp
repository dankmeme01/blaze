#pragma once
#include <modules/Module.hpp>
#include <asp/time.hpp>
#include <AsyncLoad/Manager.hpp>

namespace blaze {

class LoadModule : public Module<LoadModule> {
public:
    LoadModule();

    void onEntry();
    void onLoadingLayerInit(bool reload);
    void onLoadingLayerInitPost();
    AsyncLoad::TaskGroup onLoadStart();
    void onLoadFinished();
    void onMenuLayer();

    void loadModResourcesBlocking(std::vector<geode::Mod*> mods);

    void disableVsync();
    void restoreVsync();

private:
    asp::Instant m_processStartTime;
    asp::Instant m_entryTime;
    asp::Instant m_loadStartTime;
    asp::Instant m_loadFinishTime;
    asp::Instant m_menuLayerTime;

    void populateFpffCache();

    [[nodiscard]] AsyncLoad::TaskHandle loadImage(geode::ZStringView name);
    void loadImage(AsyncLoad::TaskGroup& group, geode::ZStringView name);

    [[nodiscard]] AsyncLoad::TaskHandle loadSheet(geode::ZStringView name);
    void loadSheet(AsyncLoad::TaskGroup& group, geode::ZStringView name);

    [[nodiscard]] AsyncLoad::TaskHandle loadFont(geode::ZStringView name);
    void loadFont(AsyncLoad::TaskGroup& group, geode::ZStringView name);
};

}
