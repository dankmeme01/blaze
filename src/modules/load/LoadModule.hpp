#pragma once
#include <modules/Module.hpp>
#include <asp/time.hpp>
#include <AsyncLoad/Manager.hpp>
#include <AsyncLoad/TaskGroup.hpp>
#include <gtl/phmap.hpp>
#include <semaphore>

namespace blaze {

struct LoadingDictionary {
    std::unique_ptr<DS_Dictionary> dict;
    std::counting_semaphore<> sem{0};
};

class LoadModule : public Module<LoadModule> {
public:
    LoadModule();

    void onPreEntry();
    void onEntry();
    void onLoadingLayerInit(bool reload);
    void onLoadingLayerInitPost();
    AsyncLoad::TaskGroup onLoadStart();
    void onLoadFinished();
    void onMenuLayer();

    void loadModResourcesBlocking(std::vector<geode::Mod*> mods);

    void disableVsync();
    void restoreVsync();

    /// Loads a DS_Dictionary* from a compressed xml file like `GManager::loadDataFromFile`.
    /// If this file was not loaded or queued yet, simply replicates that function (including handling of 2.dat)
    ///
    /// If this file is already being loaded (was enqueued before), this blocks until the result is here.
    std::unique_ptr<DS_Dictionary> loadDSDictionarySync(geode::ZStringView filename);

private:
    asp::Instant m_processStartTime;
    asp::Instant m_entryTime;
    asp::Instant m_loadStartTime;
    asp::Instant m_loadFinishTime;
    asp::Instant m_menuLayerTime;
    asp::Mutex<gtl::flat_hash_map<std::string, std::shared_ptr<LoadingDictionary>>> m_loadingDictionaries;

    void populateFpffCache();

    [[nodiscard]] AsyncLoad::TaskHandle loadImage(geode::ZStringView name);
    void loadImage(AsyncLoad::TaskGroup& group, geode::ZStringView name);

    [[nodiscard]] AsyncLoad::TaskHandle loadSheet(geode::ZStringView name);
    void loadSheet(AsyncLoad::TaskGroup& group, geode::ZStringView name);

    [[nodiscard]] AsyncLoad::TaskHandle loadFont(geode::ZStringView name);
    void loadFont(AsyncLoad::TaskGroup& group, geode::ZStringView name);

    void enqueueDSDictionaryLoad(std::string_view filename);

    std::unique_ptr<DS_Dictionary> doLoadDictionary(geode::ZStringView filename);
};

}
