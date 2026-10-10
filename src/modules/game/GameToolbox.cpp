#include <Geode/Geode.hpp>
#include <Geode/modify/GameToolbox.hpp>
#include <asp/iter.hpp>
#include "GameModule.hpp"
#include "hsv.hpp"

using namespace geode::prelude;

namespace blaze {

class $modify(GameToolbox) {
    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("GameToolbox::hsvFromString", Priority::Replace * 100);
    }

    static ccHSVValue hsvFromString(gd::string const& str, char const* delim) {
        // note: the game always passes 'a' as delim, we cannot use the actual param because windows optimizes it out
        std::string_view sv{str.c_str(), str.size()};
        return parseHsv(sv, 'a');
    }
};

}
