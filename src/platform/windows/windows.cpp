#include <Geode/Geode.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <modules/load/LoadModule.hpp>

using namespace geode::prelude;

namespace blaze {

class $modify(CCApplication) {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(
            self,
            "cocos2d::CCApplication::run"
        );

        (void) self.setHookPriority("cocos2d::CCApplication::run", Priority::First);
    }

    int run() {
        LoadModule::get().onEntry();
        return CCApplication::run();
    }
};


}

namespace blaze::platform {

asp::Instant getProcessStartTime() {
    FILETIME ftCreate, ftExit, ftKernel, ftUser;
    GetProcessTimes(GetCurrentProcess(), &ftCreate, &ftExit, &ftKernel, &ftUser);

    ULARGE_INTEGER ull;
    ull.LowPart = ftCreate.dwLowDateTime;
    ull.HighPart = ftCreate.dwHighDateTime;

    // 100-nanosecond intervals between Jan 1, 1601 and Jan 1, 1970
    const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;

    auto unix = asp::Duration::fromMicros((ull.QuadPart - EPOCH_DIFFERENCE) / 10ULL);
    auto unixNow = asp::SystemTime::now().timeSinceEpoch();
    auto delta = unixNow - unix;

    return asp::Instant::now() - delta;
}

}