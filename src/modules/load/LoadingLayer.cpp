#include "LoadingLayer.hpp"
#include <AsyncLoad/FileUtils.hpp>

using namespace geode::prelude;

namespace blaze {

bool HookedLoadingLayer::init(bool refresh) {
    LoadModule::get().onLoadingLayerInit();

    if (!LoadingLayer::init(refresh)) return false;

    // remove scheduled `loadAssets` call
    auto am = CCDirector::get()->getActionManager();
    am->removeAllActionsFromTarget(this);

    this->schedule(schedule_selector(HookedLoadingLayer::doLoad), 0.f);

    // TODO: make inter frame transitions instant, remove the vsync delay etc. while in loadinglayer

    return true;
}

void HookedLoadingLayer::doLoad(float dt) {
    auto& fields = *m_fields.self();
    auto& lm = LoadModule::get();

    if (fields.m_start) {
        lm.onLoadStart();
        fields.m_start = false;

        // Geode hooks loadAssets and thinks that no one else is calling it, so we should uphold that
        // call loadAssets once, and let Geode re-schedule it via qimt again and again until Geode is done doing its stuff
        LoadingLayer::loadAssets();
    }

    if (fields.m_shouldAdvanceLoadStep) {
        fields.m_shouldAdvanceLoadStep = false;
        LoadingLayer::loadAssets();
    }

    std::this_thread::yield();
}

// as of 2.2081, LoadingLayer::loadAssets is called 15 times:
// m_loadStep = 0 -> load GJ_GameSheet.plist -> advance to 1
// ....
// m_loadStep = 14 -> replace scene with MenuLayer
//
// since blaze rewrites the loading sequence completely to not require any intermediate steps,
// we also hook the function with a high priority to simply increment the load step and do nothing else,
// ensuring that if a certain mod hooks this function expecting e.g. loadStep 10 to load its assets, it will continue working
void HookedLoadingLayer::loadAssets() {
    auto& fields = *m_fields.self();
    auto& lm = LoadModule::get();

    bool busy = lm.runningTasks() > 0;

    // if busy, do nothing but tell them to check on us again
    if (busy) {
        fields.m_shouldAdvanceLoadStep = true;
        return;
    }

    // if we are here, all tasks are done and Geode also finished all the loading steps,
    // meaning the only thing remaining is to run the function through all remaining loading steps,
    // and finally open MenuLayer.

    bool willReplaceScene = m_loadStep == 14;
    m_loadStep = std::clamp(m_loadStep + 1, 0, 14);

    if (fields.m_prevBusy) {
        fields.m_prevBusy = false;
        lm.onLoadFinished();
    }

    // replicate original menulayer sequence too
    if (willReplaceScene) {
        LoadingLayer::loadAssets();
        return;
    }

    // run loadAssets again
    fields.m_shouldAdvanceLoadStep = true;

}

}
