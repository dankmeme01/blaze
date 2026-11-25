#include "CCArray.hpp"

namespace blaze {

static Ref<CCArray> createArray(size_t capacity = 1) {
    auto arr = new CCArray(capacity);
    return Ref<CCArray>::adopt(arr);
}

Ref<CCArray> ArrayExt::create() {
    return createArray();
}

Ref<CCArray> ArrayExt::createWithCapacity(size_t capacity) {
    return createArray(capacity);
}

}
