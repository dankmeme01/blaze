#include "CCArray.hpp"

namespace blaze {

static Ref<CCArray> createArray() {
    auto arr = (CCArray*) operator new(sizeof(CCArray));
    new (arr) CCObject();
    arr->data = nullptr;
    return Ref<CCArray>::adopt(arr);
}

Ref<CCArray> ArrayExt::create() {
    auto ret = createArray();
    ret->init();
    return ret;
}

Ref<CCArray> ArrayExt::createWithCapacity(size_t capacity) {
    auto ret = createArray();
    ret->initWithCapacity(capacity);
    return ret;
}

}
