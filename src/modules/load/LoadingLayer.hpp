#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include "LoadModule.hpp"

namespace blaze {

struct HookedLoadingLayer : geode::Modify<HookedLoadingLayer, LoadingLayer> {
    struct Fields {
        std::optional<AsyncLoad::TaskGroup> m_taskGroup;
        bool m_complete = false;
        bool m_shouldAdvanceLoadStep = false;
    };

    static void onModify(auto& self) {
        LoadModule::get().addHooks(self);

        (void) self.setHookPriority("LoadingLayer::loadAssets", geode::Priority::Replace * 10);
        (void) self.setHookPriority("LoadingLayer::init", geode::Priority::Early);
    }

    $override
    bool init(bool refresh);

    $override
    void loadAssets();

    void doLoad(float dt);
    void doFinishLoad(float dt);
};

}
