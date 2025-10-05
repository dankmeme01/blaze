#include <Geode/Geode.hpp>
#include <Geode/modify/CCSpriteBatchNode.hpp>
#include <Geode/modify/CCSprite.hpp>

#include <asp/time/Instant.hpp>

using namespace geode::prelude;
using namespace asp::time;

static Duration g_accumulatedVisitTime{};
static Duration g_accumulatedDrawTime{};
static size_t g_dirtySprites = 0;
static size_t g_notDirtySprites = 0;
static size_t g_counter = 0;

namespace blaze {

class $modify(CCSpriteBatchNode) {
    void visit() {
        auto time = Instant::now();

        if (!m_bVisible) return;

        kmGLPushMatrix();

        if (m_pGrid && m_pGrid->isActive()) {
            m_pGrid->beforeDraw();
            this->transformAncestors();
        }

        this->sortAllChildren();
        auto p1 = Instant::now();
        this->transform();
        auto p2 = Instant::now();
        this->draw();
        auto p3 = Instant::now();

        if (m_pGrid && m_pGrid->isActive()) {
            m_pGrid->afterDraw(this);
        }

        kmGLPopMatrix();

        // auto p4 = Instant::now();
        // auto elapsed = time.elapsed();
        // g_accumulatedVisitTime += elapsed;
        // g_accumulatedDrawTime += p3.durationSince(p2);

        // if (elapsed.micros() > 50) {
        //     log::debug(
        //         "Batch node visit took {} (sort: {}, transform: {}, draw: {})",
        //         elapsed.toString(),
        //         (p1.durationSince(time)).toString(),
        //         (p2.durationSince(p1)).toString(),
        //         (p3.durationSince(p2)).toString()
        //     );
        // }
    }

    void draw() {
        if (m_pobTextureAtlas->getTotalQuads() == 0) return;

        auto now = Instant::now();

        // expansion of CC_NODE_DRAW_SETUP
        ccGLEnable(m_eGLServerState);
        m_pShaderProgram->use();
        m_pShaderProgram->setUniformsForBuiltins();
        // end expansion

        auto p1 = Instant::now();

        for (auto child : CCArrayExt<CCSprite>(m_pChildren)) {
            // child->isDirty() ? g_dirtySprites++ : g_notDirtySprites++;
            child->updateTransform();
        }

        auto p2 = Instant::now();

        ccGLBlendFunc(m_blendFunc.src, m_blendFunc.dst);

        m_pobTextureAtlas->drawQuads();

        auto p3 = Instant::now();
        auto elapsed = now.elapsed();
        if (elapsed.micros() > 50) {
            // log::debug(
            //     "Batch node draw took {} (update: {}, draw: {})",
            //     elapsed.toString(),
            //     (p2.durationSince(p1)).toString(),
            //     (p3.durationSince(p2)).toString()
            // );
        }
    }
};

// void setDirtyHook(CCSprite* self, bool dirty) {
//     bool matters = self->m_pParent && self->m_pParent->m_pParent;
//     if (dirty && matters) {
//         log::debug("Dirtifying {} (parent: {})", self, self->getParent());

//         g_counter++;
//         if (g_counter == 200000) {
//             __debugbreak();
//         }
//     }

//     self->CCSprite::setDirty(dirty);
// }

// $execute {
//     Mod::get()->hook(
//         (void*)(base::getCocos() + 0x04bc40),
//         &setDirtyHook,
//         "CCSprite::setDirty"
//     ).unwrap();
// }

struct FakeGM : public CCObject {
    void printTime(float) {
        // log::debug("Accumulated visit time: {}, of which draw time: {}", g_accumulatedVisitTime.toString(), g_accumulatedDrawTime.toString());
        // log::debug("Dirty sprites: {}, not dirty: {}", g_dirtySprites, g_notDirtySprites);
    }
};

// $on_mod(Loaded) {
//     auto gm = GameManager::get();
//     gm->schedule(schedule_selector(FakeGM::printTime), 5.f);
//     gm->onEnter();
// }

}
