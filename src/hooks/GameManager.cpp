// Nothing done here due to the biggest bottleneck being parsing the savefile (which cant be easily sped up)

#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <util.hpp>
#include <TaskTimer.hpp>

using namespace geode::prelude;

#ifdef __clang__
# define B_floorf __builtin_floorf
#else
# define B_floorf std::floorf
#endif

// class $modify(GameManager) {
//     static void onModify(auto& self) {
//         BLAZE_HOOK_VERY_LAST(GameManager::init);
//     }

//     bool init() {
//         BLAZE_TIMER_START("(GameManager::init) setup");

//         m_fileName = "CCGameManager.dat";

//         m_unkBool2 = false;
//         m_vsyncEnabled = false;
//         m_adTimer = 0.0;
//         m_unkDouble2 = 0.0;
//         m_googlePlaySignedIn = false;
//         m_unkArray = CCArray::create();
//         m_unkArray->retain();

//         this->setupReimpl();

//         BLAZE_TIMER_STEP("Misc");

//         this->calculateBaseKeyForIconsReimpl();
//         this->setupGameAnimationsReimpl();

//         // i think this is right but i have no idea tbh
//         float fVar8 = B_floorf((float)m_playerUserID.value() / 10000.f);
//         auto iVar2 = CCRANDOM_0_1() * 1000.f;
//         auto someNumber = (int)(fVar8 + 1000000.f * (CCRANDOM_0_1()));
//         m_chk = SeedValueRS(someNumber, iVar2);

//         BLAZE_TIMER_END();

//         return true;
//     }

//     void calculateBaseKeyForIconsReimpl() {
// #if GEODE_COMP_GD_VERSION == 22060
//         m_keyStartForIcon.resize(9);

//         int counter = 0;
//         m_keyStartForIcon[0] = counter;
//         m_keyStartForIcon[1] = counter += 0x1e4;
//         m_keyStartForIcon[2] = counter += 0xa9;
//         m_keyStartForIcon[3] = counter += 0x76;
//         m_keyStartForIcon[4] = counter += 0x95;
//         m_keyStartForIcon[5] = counter += 0x60;
//         m_keyStartForIcon[6] = counter += 0x44;
//         m_keyStartForIcon[7] = counter += 0x45;
//         m_keyStartForIcon[8] = counter += 0x2b;
//         counter += 0x5;
//         // m_keyStartForIcon[9] = 0x5; ?
// #else
// # error "Incompatible GD version, please update the icon counts"
// #endif

//         for (size_t i = 0; i < counter; i++) {
//             m_loadIcon[i] = 0;
//         }
//     }

//     void setupGameAnimationsReimpl() {
//         GameManager::setupGameAnimations();
//     }

//     void setupReimpl() {
//         m_setup = true;
//         this->loadDataFromFile(m_fileName);
//     }
// };
