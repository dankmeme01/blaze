#pragma once

#include <asp/sync/Mutex.hpp>

#include "load/spriteframes.hpp"

namespace blaze {
    extern asp::Mutex<> g_sfcacheMutex;

    // Parses data from a .plist file into a structure holding many sprite frames.
    // Will cache them, if the plist has already been loaded, it will not be reloaded.
    std::shared_ptr<SpriteFrameData> loadSpriteFrames(const char* path);
}
