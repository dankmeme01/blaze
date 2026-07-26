#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include "LoadModule.hpp"

namespace blaze {

struct HookedLoadingLayer : geode::Modify<HookedLoadingLayer, LoadingLayer> {
    struct Fields {
        bool m_start = true;
        bool m_prevBusy = true;
        bool m_shouldAdvanceLoadStep = false;
    };

    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "LoadingLayer::init",
            "LoadingLayer::loadAssets"
        );

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
