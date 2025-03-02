#include "thread.hpp"

namespace blaze {

std::thread::id mainThreadId{};

void setMainThreadId() {
    mainThreadId = std::this_thread::get_id();
}

bool isMainThread() {
    return std::this_thread::get_id() == mainThreadId;
}

} // namespace blaze