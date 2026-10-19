#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <asp/iter.hpp>
#include <asp/thread/ThreadPool.hpp>
#include "GameModule.hpp"
#include "object/ObjectLoader.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(BlazeGamePL, PlayLayer) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("PlayLayer::processCreateObjectsFromSetup", Priority::Replace * 100);
        (void) self.setHookPriority("PlayLayer::prepareCreateObjectsFromSetup", Priority::Replace * 100);
    }

    struct Fields {
        std::optional<ObjectLoader> m_loader;
    };

    $override
    void prepareCreateObjectsFromSetup(std::string& levelString) {
        std::string_view sv{levelString.c_str(), levelString.size()};

        if (!sv.empty() && sv != " ") {
            m_objectStrings = asp::iter::split(sv, ';')
                .filter([](std::string_view s) { return !s.empty(); })
                .map([](std::string_view s) { return std::string{s.data(), s.size()}; })
                .collect<std::vector<std::string>>();

            m_levelSettings = LevelSettingsObject::objectFromString(m_objectStrings[0]);
            m_levelSettings->retain();
            m_levelSettings->m_level = m_level;
            this->loadLevelSettings();

            m_levelSettings->m_effectManager->updateColors(m_player1->m_playerColor1, m_player1->m_playerColor2);

            GameManager::get()->loadFont(m_levelSettings->m_fontIndex);
        }

        m_coinArray = CCArray::create();
        m_coinArray->retain();

        m_objectsCreated = 1;
        levelString.clear();

        m_fields->m_loader.emplace(this);
    }

    $override
    void processCreateObjectsFromSetup() {
        auto& fields = *m_fields.self();
        B_ASSERT(fields.m_loader);

        auto start = asp::Instant::now();
        auto& loader = *fields.m_loader;
        loader.processBatch();

        auto lprog = loader.progress();
        BLAZE_TRACE_NOISY("ObjectLoader progress: {} / {} ({}%)", lprog.loadedObjects, lprog.totalObjects, (int)(lprog.percent() * 100.0f));

        m_objectsCreated = lprog.loadedObjects + 1;
        m_loadingProgress = lprog.percent();

        if (lprog.complete()) {
            loader.complete();
            fields.m_loader.reset();

            this->createObjectsFromSetupFinished();
            this->setupHasCompleted();
        } else {
            loader.sleep(start.elapsed());
        }
    }
};

}
