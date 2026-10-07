#include <Geode/Geode.hpp>
#include <Geode/modify/CCAutoreleasePool.hpp>
#include <Geode/modify/CCPoolManager.hpp>
#include <Geode/modify/CCObject.hpp>
#include <util/assert.hpp>
#include <Geode/utils/terminate.hpp>
#include "EngineModule.hpp"

using namespace geode::prelude;

namespace blaze {

struct Pool {
    // no refs so we control everything carefully ourselves
    std::vector<CCObject*> m_objects;

    static inline int s_sams = 0;
    static inline int s_sprites = 0;

    void addObject(CCObject* obj) {
        obj->m_uAutoReleaseCount++;
        m_objects.push_back(obj);
        // log::debug("addObject({} @ {}) new count: {}", obj, (void*)obj, obj->m_uAutoReleaseCount);
    }

    void removeObject(CCObject* obj) {
        // log::debug("removeObject({})", obj);
        uint32_t count = obj->m_uAutoReleaseCount;
        if (count == 0) return;

        size_t w = 0;
        for (size_t r = 0; r < m_objects.size(); r++) {
            auto cur = m_objects[r];
            if (cur == obj && count > 0) {
                count--;
                continue;
            }

            m_objects[w++] = cur;
        }

        m_objects.resize(w);
        obj->m_uAutoReleaseCount = 0;
    }

    void clear() {
        while (!m_objects.empty()) {
            auto obj = m_objects.back();
            m_objects.pop_back();

            obj->m_uAutoReleaseCount--;
            obj->release();
        }
    }
};

struct CCPoolManagerFields {
    CCArray* m_pReleasePoolStack;
    CCAutoreleasePool* m_pCurReleasePool;
};

class $modify(CCPoolManagerHook, CCPoolManager) {
    inline static std::vector<Pool> s_stack;
    inline static std::optional<Pool> s_cachedPool;

    static void onModify(auto& self) {
        EngineModule::get().addHooks(self);

        (void) self.setHookPriority("cocos2d::CCPoolManager::addObject", Priority::Replace * 1000);
        (void) self.setHookPriority("cocos2d::CCPoolManager::removeObject", Priority::Replace * 1000);
        (void) self.setHookPriority("cocos2d::CCPoolManager::push", Priority::Replace * 1000);
        (void) self.setHookPriority("cocos2d::CCPoolManager::pop", Priority::Replace * 1000);
        (void) self.setHookPriority("cocos2d::CCPoolManager::finalize", Priority::Replace * 1000);
    }

    $override
    void addObject(CCObject* obj) {
        this->getPool().addObject(obj);
    }

    $override
    void removeObject(CCObject* obj) {
        this->getPool().removeObject(obj);
    }

    $override
    void push() {
        s_stack.emplace_back(this->getNewOrCached());
    }

    $override
    void pop() {
        if (s_stack.empty()) return;

        // upon popping place the Stack structure in the cache to reuse the memory of the umap
        s_stack.back().clear();
        s_cachedPool = std::move(s_stack.back());
        s_stack.pop_back();
    }

    $override
    void finalize() {
        // this in cocos apparently just clears out all pools but does not remove them ?
        // do more cleanup because dtor is unhookable on windows
        for (auto& pool : s_stack) {
            pool.clear();
        }

        // do some dirty cleanup for the vanilla fields
        auto fields = (CCPoolManagerFields*)this;
        fields->m_pCurReleasePool = nullptr;

        // set counts to 0 so it early returns from Pool::removeObject, otherwise risks triggering logs
        for (auto pool : fields->m_pReleasePoolStack->asExt<CCAutoreleasePool>()) {
            for (auto obj : pool->m_pManagedObjectArray->asExt()) {
                obj->m_uAutoReleaseCount = 0;
            }
            pool->m_uAutoReleaseCount = 0;
        }
        fields->m_pReleasePoolStack->removeAllObjects();

        // leave a single dummy object because vanilla has to call removeObjectAtIndex(0);
        fields->m_pReleasePoolStack->addObject(Ref<CCObject>::adopt(new CCObject()));

        s_stack.clear();
        s_cachedPool.reset();
    }

    static Pool& getPool() {
        if (!s_stack.empty()) {
            return s_stack.back();
        }

        return s_stack.emplace_back(getNewOrCached());
    }

    static Pool getNewOrCached() {
        if (s_cachedPool) {
            auto pool = std::move(*s_cachedPool);
            s_cachedPool.reset();
            return pool;
        }

        return Pool{};
    }
};

// CCObject::autorelease simply calls CCPoolManager::addObject, but we can also hook it and call our version directly.
// This is a micro optimization because Alpha's Geode Utils already hooks autorelease, so we avoid having 2 separate hooks in the path
class $modify(CCAutoreleaseObjectHook, CCObject) {
    static void onModify(auto& self) {
        EngineModule::get().addHooks(self);

        (void) self.setHookPriority("cocos2d::CCObject::autorelease", Priority::Replace * 1000);
    }

    $override
    CCObject* autorelease() {
        CCPoolManagerHook::getPool().addObject(this);
        return this;
    }
};

}

// benchmark
// $on_mod(Loaded) {
//     auto t1 = asp::Instant::now();
//     for (size_t i = 0; i < 4 * 1024 * 1024; i++) {
//         auto obj = new CCObject();
//         obj->autorelease();
//     }
//     auto t2 = asp::Instant::now();
//     CCPoolManager::sharedPoolManager()->pop();
//     auto t3 = asp::Instant::now();

//     log::debug("creation took {}, pop took {}", t2.durationSince(t1), t3.durationSince(t2));
// }