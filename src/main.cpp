#include <Geode/Geode.hpp>

#include <fpng.h>
#include <pugixml.hpp>

using namespace geode::prelude;

namespace blaze {
    void* pugiAlloc(size_t size) {
        return new (std::nothrow) uint8_t[size];
    }

    void pugiFree(void* ptr) {
        delete[] static_cast<uint8_t*>(ptr);
    }
}

$execute {
    fpng::fpng_init();

    pugi::set_memory_management_functions(&blaze::pugiAlloc, &blaze::pugiFree);
}
