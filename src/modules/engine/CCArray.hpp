#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/CCArray.hpp>
#include "EngineModule.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(ArrayExt, CCArray) {
    static void onModify(auto& self) {
        EngineModule::get().addHooks(self);
    }

    static Ref<CCArray> create();
    static Ref<CCArray> createWithCapacity(size_t);
};

}

