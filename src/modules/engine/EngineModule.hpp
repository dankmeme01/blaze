#pragma once
#include <modules/Module.hpp>

namespace blaze {

/// Module for game engine optimizations, aka fixes for various cocos stuff that does not fall into other categories
class EngineModule : public Module<EngineModule> {
public:
    EngineModule();

private:
};

}
