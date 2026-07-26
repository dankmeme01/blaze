#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/LoadingLayer.hpp>
#include "LoadModule.hpp"

namespace blaze {

struct HookedLoadingLayer : geode::Modify<HookedLoadingLayer, LoadingLayer> {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "LoadingLayer::init",
            "LoadingLayer::loadAssets"
        );
    }

    $override
    bool init(bool refresh);
    $override
    void loadAssets();
};

}
