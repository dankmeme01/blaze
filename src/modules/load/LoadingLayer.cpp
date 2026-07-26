#include "LoadingLayer.hpp"

using namespace geode::prelude;

namespace blaze {

bool HookedLoadingLayer::init(bool refresh) {
    return LoadingLayer::init(refresh);
}

void HookedLoadingLayer::loadAssets() {
    LoadingLayer::loadAssets();
}

}
