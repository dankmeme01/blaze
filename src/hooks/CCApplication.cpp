#include <Geode/Geode.hpp>

#ifdef GEODE_IS_WINDOWS

#include <util/assert.hpp>
#include <util.hpp>
#include <Geode/modify/CCApplication.hpp>
#include <Geode/modify/CCObject.hpp>
#include <Windows.h>
#include <timeapi.h>

using namespace geode::prelude;

static void PVRFrameEnableControlWindow(bool bEnable) {
    HKEY hKey = 0;

    if(ERROR_SUCCESS != RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Imagination Technologies\\PVRVFRame\\STARTUP\\",
        0,
        0,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        0,
        &hKey,
        nullptr))
    {
        return;
    }

    auto wszNewData = (bEnable) ? L"NO" : L"YES";
    WCHAR wszOldData[256] = {0};
    DWORD dwSize = sizeof(wszOldData);

    LSTATUS status = RegQueryValueExW(hKey, L"hide_gui", nullptr, nullptr, (BYTE*)wszOldData, &dwSize);

    if (ERROR_FILE_NOT_FOUND == status              // the key not exist
        || (ERROR_SUCCESS == status                 // or the hide_gui value is exist
        && 0 != wcscmp(wszNewData, wszOldData)))    // but new data and old data not equal
    {
        dwSize = sizeof(WCHAR) * (wcslen(wszNewData) + 1);
        RegSetValueExW(hKey, L"hide_gui", 0, REG_SZ, (BYTE  *)wszNewData, dwSize);
    }

    RegCloseKey(hKey);
}

static auto& s_accumulatedDelta = *(float*)(geode::base::getCocos() + 0x1a5490);
static auto& s_frameCount = *(int*)(geode::base::getCocos() + 0x1a5494);
static auto& s_someDat98 = *(long long*)(geode::base::getCocos() + 0x1a5498);

class MyDirector : public CCDirector {
public:
    void mainLoop() { BLAZE_ASSERT(false); }
};

static_assert(140 == offsetof(CCEGLView, m_uReference), "Outdated Geode");

static double g_tsc_hz = 0.0;

enum class SleepVariant {
    TPause,
    Mwaitx,
    Pause,
};

using SleepFn = void(*)(uint32_t);
static SleepFn g_sleep = nullptr;

void calibrate_tsc();
void initSleep();

static __forceinline uint64_t rdtsc() {
    unsigned int aux;
    return __rdtscp(&aux);
}

static __forceinline uint32_t ns_to_cycles(uint64_t ns) {
    return (uint32_t)((double)ns * g_tsc_hz / 1'000'000'000.0);
}

static __forceinline uint64_t cycles_to_ns(uint32_t cycles) {
    return (uint64_t)((double)cycles * 1'000'000'000.0 / g_tsc_hz);
}

static __forceinline uint64_t qpc_to_cycles(LARGE_INTEGER qpc, LARGE_INTEGER freq) {
    // convert to ns first
    uint64_t ns = (uint64_t)((double)qpc.QuadPart * 1'000'000'000.0 / (double)freq.QuadPart);
    return ns_to_cycles(ns);
}

class $modify(CCApplication) {
    static void onModify(auto& self) {
        BLAZE_HOOK_VERY_LAST(cocos2d::CCApplication::run);
    }

    int run() {
        // pin the main thread to first cpu core for debugging
        // SetThreadAffinityMask(GetCurrentThread(), 1ull);

        calibrate_tsc();
        initSleep();

        PVRFrameEnableControlWindow(false);
        this->setupGLView();

        auto director = CCDirector::get();
        auto glView = director->m_pobOpenGLView;
        glView->retain();

        this->setupVerticalSync();
        this->updateVerticalSync();

        if (!this->applicationDidFinishLaunching()) {
            return 0;
        }

        bool fullscreen = glView->m_bIsFullscreen;
        m_bFullscreen = fullscreen;

        uint64_t lastCounter = rdtsc();
        uint64_t currentCounter;

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        auto yieldDeadline = ns_to_cycles(1'000'000); // only call Sleep if >= 1 ms to wait

        timeBeginPeriod(1);

        while (!glView->windowShouldClose()) {
            if (m_bFullscreen != fullscreen) {
                this->updateVerticalSync();
                m_bForceTimer = false;
                fullscreen = m_bFullscreen;
            }

            float targetInterval;
            uint64_t targetTicks;

            if (m_bVerticalSyncEnabled) {
                targetInterval = m_fVsyncInterval;
                targetTicks = qpc_to_cycles(m_nVsyncInterval, frequency);
            } else {
                targetInterval = m_fAnimationInterval;
                targetTicks = qpc_to_cycles(m_nAnimationInterval, frequency);
            }

            currentCounter = rdtsc();
            uint64_t elapsedTicks = currentCounter - lastCounter;
            uint64_t ticksToWait = targetTicks - elapsedTicks;

            bool shouldProcessFrame = (!m_bFullscreen && !m_bForceTimer) || (elapsedTicks >= targetTicks);
            if (!shouldProcessFrame) {
                // if there's a lot of time to wait, yield to the OS
                if (ticksToWait >= yieldDeadline) {
                    DWORD sleepMs = (DWORD)(cycles_to_ns(ticksToWait) / 1'000'000);
                    // log::debug("Sleeping for {} ms", sleepMs);
                    Sleep(sleepMs == 0 ? 0 : sleepMs - 1);

                    currentCounter = rdtsc();
                    elapsedTicks = currentCounter - lastCounter;
                }

                // perform a very short sleep for the remaining time
                while (elapsedTicks < targetTicks) {
                    ticksToWait = targetTicks - elapsedTicks;
                    g_sleep(ticksToWait);

                    currentCounter = rdtsc();
                    elapsedTicks = currentCounter - lastCounter;
                }
            }

            s_someDat98 = currentCounter;

            // update the controller
            if (m_bUpdateController) {
                // TODO: update controller func does not exist
                // m_pControllerHandler->updateConnected();
                // m_pController2Handler->updateConnected();
                m_bUpdateController = false;
            }

            bool c1Connected = m_pControllerHandler->m_controllerConnected;
            bool c2Connected = m_pController2Handler->m_controllerConnected;
            m_bControllerConnected = c1Connected || c2Connected;

            if (c1Connected) {
                this->updateControllerKeys(m_pControllerHandler, 1);
            }
            if (c2Connected) {
                this->updateControllerKeys(m_pController2Handler, 2);
            }

            float actualDeltaTime = static_cast<float>(
                (double)elapsedTicks / (double)targetTicks * (double)targetInterval
            );
            float displayDeltaTime = actualDeltaTime;

            if (!m_bFullscreen && !m_bForceTimer) {
                s_accumulatedDelta += actualDeltaTime;
                s_frameCount++;

                if (s_accumulatedDelta > 0.5f) {
                    float avgFps = (float)s_frameCount / s_accumulatedDelta;
                    s_accumulatedDelta = 0.f;
                    s_frameCount = 0;

                    if ((1.0f / targetInterval) * 1.2f < avgFps) {
                        m_bForceTimer = true;
                    }
                }
            }

            if (m_bSmoothFix && director->m_bSmoothFixCheck) {
                if (std::abs(actualDeltaTime - targetInterval) <= targetInterval * 0.1f) {
                    displayDeltaTime = targetInterval;
                }
            }

            // poll events and render
            glView->pollEvents();
            director->setDeltaTime(displayDeltaTime);
            director->m_fActualDeltaTime = actualDeltaTime;
            reinterpret_cast<MyDirector*>(director)->mainLoop();

            lastCounter = currentCounter;
        }

        timeEndPeriod(1);

        if (!m_bShutdownCalled) {
            CCApplication::get()->trySaveGame(false);
        }

        if (glView->isOpenGLReady()) {
            director->end();
            reinterpret_cast<MyDirector*>(director)->mainLoop();
        }

        glView->release();
        this->platformShutdown();

        return 0;
    }
};

static void __attribute__((target("mwaitx"))) sl_mwaitx(uint32_t cycles)  {
    constexpr uint32_t timer_enable = 0x2;

    alignas(64) uint64_t monitorVar = 0;
    _mm_monitorx(&monitorVar, 0, 0);
    _mm_mwaitx(timer_enable, 0, cycles);
}

static void __attribute__((target("waitpkg"))) sl_tpause(uint32_t cycles)  {
    constexpr uint32_t cstate = 0x0;
    _tpause(cstate, rdtsc() + cycles);
}

static void sl_pause(uint32_t cycles) {
    uint64_t end = rdtsc() + cycles;
    while (rdtsc() < end) {
        _mm_pause();
    }
}

struct CpuidResult {
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
};

static bool hasWaitpkg() {
    CpuidResult vals;
    __cpuidex((int*)&vals, 7, 0);
    return (vals.ecx & (1u << 5)) != 0;
}

static bool hasMwaitx() {
    CpuidResult vals;
    __cpuidex((int*)&vals, 0x80000001, 0);
    return (vals.ecx & (1u << 29)) != 0;
}

static SleepVariant getBestVariant() {
    if (hasWaitpkg()) {
        return SleepVariant::TPause;
    } else if (hasMwaitx()) {
        return SleepVariant::Mwaitx;
    }

    return SleepVariant::Pause;
}

void initSleep() {
    switch (getBestVariant()) {
        case SleepVariant::TPause: {
            log::debug("Using TPAUSE for sleeping");
            g_sleep = sl_tpause;
        } break;
        case SleepVariant::Mwaitx: {
            log::debug("Using MONITORX for sleeping");
            g_sleep = sl_mwaitx;
        } break;
        case SleepVariant::Pause: [[fallthrough]];
        default: {
            log::debug("Using PAUSE loop for sleeping");
            g_sleep = sl_pause;
        } break;
    }
}

void calibrate_tsc() {
    LARGE_INTEGER freq;
    LARGE_INTEGER start, end;

    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    uint64_t tsc_start = rdtsc();

    do {
        QueryPerformanceCounter(&end);
    } while ((end.QuadPart - start.QuadPart) < (freq.QuadPart / 10));

    uint64_t tsc_end = rdtsc();

    double elapsed_sec =
        (double)(end.QuadPart - start.QuadPart) /
        (double)freq.QuadPart;

    g_tsc_hz = (double)(tsc_end - tsc_start) / elapsed_sec;
    log::info("TSC frequency: {:.2f}", g_tsc_hz);
}

#endif