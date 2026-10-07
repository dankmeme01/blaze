#include <Geode/modify/AppDelegate.hpp>
#include <modules/load/LoadModule.hpp>
#include <unistd.h>
#include <sys/sysinfo.h>

using namespace geode::prelude;

namespace blaze {

class $modify(AppDelegate) {
    static void onModify(auto& self) {
        LoadModule::get().addHooks(self);

        (void) self.setHookPriority("AppDelegate::applicationDidFinishLaunching", Priority::First);
    }

    bool applicationDidFinishLaunching() {
        LoadModule::get().onEntry();
        return AppDelegate::applicationDidFinishLaunching();
    }
};

}

namespace blaze::platform {

asp::Instant getProcessStartTime() {
    return asp::Instant::now();
}

// Note: the code below does work, but it measures start time of the geode launcher, not GD activity specifically.
// Thus it is kind of pointless for measuring OS overhead, because it will always include the time spent in the launcher.

#if 0
asp::Instant getProcessStartTime() {
    // for weird reasons, readString simply succeeds with an empty string here, so use standard io
    char buf[1024];
    auto fp = fopen("/proc/self/stat", "r");
    if (!fp) return asp::Instant::now();

    fgets(buf, sizeof(buf), fp);
    fclose(fp);

    std::string_view str = buf;

    // example of `cat /proc/self/stat`:
    // 17603 (cat) R 17592 17603 17592 34816 17603 4194304 519 0 7 0 1 3 0 0 20 0 1 0 1793715 11085021184 941 18446744073709551615 417965195264 417965618544 548732962672 0 0 0 0 0 1073775864 0 0 0 17 1 0 0 0 0 0 417965654016 417965665328 418827866112 548732963638 548732963658 548732963658 548732968936 0

    // find last paren
    auto rparen = str.find_last_of(')');
    if (rparen == std::string::npos) return asp::Instant::now();


    auto rem = std::string_view{str}.substr(rparen);
    auto num = asp::iter::split(rem, ' ')
        .skip(20)
        .take(1)
        .filterMap([](auto sv) { return utils::numFromString<uint64_t>(sv).ok(); })
        .next();

    if (!num) return asp::Instant::now();

    uint64_t ticks = *num;

    auto perSec = sysconf(_SC_CLK_TCK);
    double secs = (double)ticks / (double)perSec;

    struct sysinfo info;
    sysinfo(&info);

    double uptimeSecs = (double)info.uptime - secs;
    return asp::Instant::now() - *asp::Duration::fromSecs(uptimeSecs);
}
#endif

}