#pragma once
#include <modules/Module.hpp>
#include <asp/time.hpp>
#include <AsyncLoad/Manager.hpp>

namespace blaze {

class LoadModule : public Module<LoadModule> {
public:
    LoadModule();

    void onEntry();
    void onLoadingLayerInit();
    void onLoadStart();
    void onLoadFinished();
    void onMenuLayer();

    void loadModResourcesBlocking(std::vector<geode::Mod*> mods);

    size_t runningTasks() const;

private:
    asp::Instant m_entryTime;
    asp::Instant m_loadStartTime;
    asp::Instant m_loadFinishTime;
    asp::Instant m_menuLayerTime;
    size_t m_awaitingTasks = 0;

    void populateFpffCache();
    void loadImage(geode::ZStringView name);
    void loadSheet(geode::ZStringView name);
    void loadFont(geode::ZStringView name);
};

}
