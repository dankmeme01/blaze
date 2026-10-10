#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <asp/iter.hpp>
#include "GameModule.hpp"
#include "GameObject.hpp"

using namespace geode::prelude;

namespace blaze {

static int fastParseKey(std::string_view sv) {
    int val = 0;
    for (char c : sv) {
        if (c < '0' || c > '9') [[unlikely]] {
            return 0;
        }
        val = val * 10 + (c - '0');
    }
    return val;
}

class $modify(BlazeGamePL, PlayLayer) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("PlayLayer::processCreateObjectsFromSetup", Priority::Replace * 100);
        (void) self.setHookPriority("PlayLayer::prepareCreateObjectsFromSetup", Priority::Replace * 100);
    }

    struct Fields {
        asp::Instant m_loadStart;
    };

    $override
    void prepareCreateObjectsFromSetup(gd::string& levelString) {
        m_fields->m_loadStart = asp::Instant::now();

        std::string_view sv{levelString.c_str(), levelString.size()};

        if (!sv.empty() && sv != " ") {
            m_objectStrings = asp::iter::split(sv, ';')
                .filter([](std::string_view s) { return !s.empty(); })
                .map([](std::string_view s) { return gd::string{s.data(), s.size()}; })
                .collect<gd::vector<gd::string>>();

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
    }

    $override
    void processCreateObjectsFromSetup() {
        size_t totalObjects = m_objectStrings.size();
        int objectsCreated = m_objectsCreated;

        double batches = std::min(std::floor(totalObjects / 1000.0) + 10.0, 100.0);
        size_t objectsPerBatch = std::ceil(totalObjects / batches);

        if (objectsCreated < totalObjects) {
            this->buildBatch(objectsCreated, objectsPerBatch);
        }

        float progress = std::min(1.0f, (float)(m_objectsCreated - 1) / (float)totalObjects);
        m_loadingProgress = progress;

        if (m_objectsCreated >= totalObjects) {
            this->createObjectsFromSetupFinished();
            m_loadingProgress = 1.0f;
            this->setupHasCompleted();

#ifdef BLAZE_DEBUG
            auto took = m_fields->m_loadStart.elapsed();
            log::info("Took {} to load the level!", took);
#endif
        }
    }

    void buildBatch(size_t startIndex, size_t batchSize) {
        ObjectProps props;

        auto levelType = m_level->m_levelType;
        bool ldm = m_level->m_lowDetailModeToggled;

        size_t totalObjects = m_objectStrings.size();
        size_t endIndex = std::min(startIndex + batchSize, totalObjects);

        for (size_t index = startIndex; index < endIndex; index++) {
            auto& str = m_objectStrings[index];

            props.reset();

            // string in format k,v,k,v
            for (auto [k, v] : asp::iter::split(str, ',').arrayChunks<2>()) {
                int key = fastParseKey(k);
                if (key >= 1 && key <= 600) {
                    props.set(key, v);
                }
            }

            // auto obj = BlazeGameObject::fromValues(props, this, ldm);
            auto obj = GameObject::objectFromVector(props.m_values, props.m_present, this, ldm);
            if (obj) {
                this->postBuildObjectSetup(obj, str, levelType);
            }

            str.clear();
        }

        m_objectsCreated = endIndex;
    }

    void postBuildObjectSetup(GameObject* obj, const gd::string& objectStr, GJLevelType levelType) {
        // gold coins can only appear in main levels
        if (obj->getType() == GameObjectType::SecretCoin && levelType != GJLevelType::Main) {
            return;
        }

        // hard coded rob stuff
        if (obj->m_objectID == 31) {
            static_cast<StartPosObject*>(obj)->loadSettingsFromString(objectStr);
        } else if (obj->m_objectID == 2065) {
            static_cast<ParticleGameObject*>(obj)->updateParticleStruct();
        }

        if (obj->m_mainColorKeyIndex > 0) {
            if (obj->m_mainColorKeyIndex > m_nextColorKey) {
                m_nextColorKey = obj->m_mainColorKeyIndex;
            }
            if (obj->m_detailColorKeyIndex > m_nextColorKey) {
                m_nextColorKey = obj->m_detailColorKeyIndex;
            }
        } else {
            // allocate color channel for each active color key and store back in the object
            int colorCount = obj->hasSecondaryColor() ? 2 : 1;
            for (int i = 0; i < colorCount; i++) {
                auto colorKey = obj->getColorKey(i == 0, false);

                int colorIndex;
                if (auto* existing = m_colorKeyDict->objectForKey(colorKey)) {
                    colorIndex = static_cast<CCInteger*>(existing)->m_nValue;
                } else {
                    colorIndex = m_nextColorKey;
                    m_colorKeyDict->setObject(CCInteger::create(colorIndex), colorKey);
                    m_nextColorKey++;
                }

                if (i == 0) {
                    obj->m_mainColorKeyIndex = colorIndex;
                } else {
                    obj->m_detailColorKeyIndex = colorIndex;
                }
            }
        }

        // only first three use coins are reigstered
        if (obj->getType() == GameObjectType::UserCoin) {
            if (m_coinArray->count() < 3) {
                m_coinArray->addObject(obj);
                this->addObject(obj);
            }
        } else {
            this->addObject(obj);
        }
    }
};

}
