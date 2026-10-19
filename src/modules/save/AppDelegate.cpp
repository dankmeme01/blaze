#include "SaveModule.hpp"
#include <semaphore>
#include <Geode/modify/AppDelegate.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(AppDelegate) {
    static void onModify(auto& self) {
        SaveModule::get().addHooks(self);

        (void) self.setHookPriority("AppDelegate::trySaveGame", Priority::Replace * 10);
    }

    void trySaveGame(bool force) {
        // maintain silly vanilla behavior
        if (force) {
            m_saveTime = 0.0;
        }

        bool shouldSave = m_saveTime == 0.0;

        if (!shouldSave) {
            auto saveTime = asp::SystemTime::UNIX_EPOCH + asp::Duration::fromSecsF32(m_saveTime);
            shouldSave = saveTime.elapsed() > asp::Duration::fromSecs(10);
        }

        if (!shouldSave) {
            return;
        }

        if (m_glViewSetup) {
            this->doSave();

            m_saveTime = asp::SystemTime::now().timeSinceEpoch().seconds<float>();
        }

        PlatformToolbox::gameDidSave();
    }

    void doSave() {
        // Original:
        GameManager::get()->save();
        LocalLevelManager::get()->save();

        // TODO: technically unsound due to autorelease
        // std::counting_semaphore<> sem{0};

        // arc::spawnBlocking<void>([&] {
        //     GameManager::get()->save();
        //     sem.release();
        // });

        // arc::spawnBlocking<void>([&] {
        //     LocalLevelManager::get()->save();
        //     sem.release();
        // });

        // sem.acquire();
        // sem.acquire();
    }
};

}
