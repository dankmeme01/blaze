#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "GameModule.hpp"

using namespace geode::prelude;

namespace blaze {

// class $modify(BlazeGameGJBGL, GJBaseGameLayer) {
//     static void onModify(auto& self) {
//         GameModule::get().addHooks(self, "GJBaseGameLayer::moveObjects");
//     }

//     $override
//     void moveObjects(CCArray* objects, double dx, double dy, bool lockPlayerY) {
//         if (!objects) return;

//         CCArrayExt<GameObject, false> arr{objects};
//         m_movedCount += arr.size();
//         auto now = asp::Instant::now();

//         auto cmd = m_gameState.m_commandIndex;
//         bool moveX = dx != 0.0;
//         bool moveY = dy != 0.0;

//         size_t k = 6;
//         for (auto [i, obj] : asp::iter::enumerate(arr)) {
//             if (i + k < arr.size()) {
//                 auto obj = arr[i + k];
//                 __builtin_prefetch(&obj->m_positionX, 1, 3);
//                 __builtin_prefetch(&obj->m_positionY, 1, 3);
//                 __builtin_prefetch(&obj->m_isDecoration2, 1, 3);
//             }

//             if (!obj->m_isDecoration2 && obj->m_unk4C4 != cmd) {
//                 obj->m_lastPosition = CCPoint{
//                     (float)obj->m_positionX, (float)obj->m_positionY
//                 };
//                 obj->m_unk4C4 = cmd;
//                 obj->dirtifyObjectRect();
//             }

//             if (moveX && !obj->m_tempOffsetXRelated) {
//                 obj->m_positionX += dx;
//             }
//             if (moveY) {
//                 obj->m_positionY += dy;
//             }

//             obj->dirtifyObjectPos();
//             this->updateObjectSection(obj);
//         }

//         this->updateDisabledObjectsLastPos(objects);

//         if (arr.size() > 100) {
//             log::debug("moved {} in {}", arr.size(), now.elapsed());
//         }
//     }
// };

}
