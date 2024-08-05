#include "FMODAudioEngine.hpp"

#include <TaskTimer.hpp>
#include <asp/thread/Thread.hpp>

#include <fmod_errors.h>

using namespace geode::prelude;

static FMOD_RESULT errcheck_fn(FMOD_RESULT r) {
    if (r != FMOD_OK) {
        log::warn("FMOD error: {}", FMOD_ErrorString(r));
    }

    return r;
}

#define FMOD_CALL(...) m_lastResult = errcheck_fn((__VA_ARGS__));

void HookedFMODAudioEngine::setupAudioEngine() {
    if (m_fields->initialized) {
        log::warn("Attempting to repeatedly initialize the audio engine");
        return;
    }

    bool gv0159 = GameManager::get()->getGameVariable("0159");
    bool reducedQuality = GameManager::get()->getGameVariable("0142");

#ifdef GEODE_IS_ANDROID
    // this bullshit os cannot initialize fmod from a thread
    this->setupAudioEngineReimpl(gv0159, reducedQuality);
#else
    asp::Thread<bool, bool> thread;

    thread.setLoopFunction([this](bool a, bool b, asp::StopToken<bool, bool>& stopToken) {
        this->setupAudioEngineReimpl(a, b);
        stopToken.stop();
    });

    thread.start(gv0159, reducedQuality);
    thread.detach();
#endif
}

void HookedFMODAudioEngine::setupAudioEngineReimpl(bool gv0159, bool reducedQuality) {
    // Blaze initializes the engine on another thread, so just to be sure, synchronize this part
    std::lock_guard lock(m_fields->initMutex);

    BLAZE_TIMER_START("FMOD::System_Create");

    if (m_fields->initialized) {
        log::warn("Attempting to repeatedly initialize the audio engine");
        return;
    }

    FMOD_CALL(FMOD::System_Create(&m_system));

    if (!m_system) {
        log::warn("FMOD System initialization failed, so the rest of the audio engine initialization cannot proceed.");
        return;
    }

    // unsigned int version;
    // m_system->getVersion(&version);

    BLAZE_TIMER_STEP("FMOD misc inits 1");

    // unsigned int bufferSize;
    // FMOD_TIMEUNIT buffersizeType;
    // m_system->getStreamBufferSize(&bufferSize, &buffersizeType);

    if (!gv0159) {
        m_system->setDSPBufferSize(512, 4);
    }

    // unsigned int bufferLength;
    // int numBuffers;
    // m_system->getDSPBufferSize(&bufferLength, &numBuffers);

    int sampleRate;
    FMOD_SPEAKERMODE speakerMode;
    int numRawSpeakers;
    m_system->getSoftwareFormat(&sampleRate, &speakerMode, &numRawSpeakers);

    this->m_reducedQuality = reducedQuality;
    sampleRate = this->m_reducedQuality ? 24000 : 44100;

    m_system->setSoftwareFormat(sampleRate, speakerMode, numRawSpeakers);

    BLAZE_TIMER_STEP("FMOD system init");

    FMOD_CALL(m_system->init(0x80, 0, nullptr));

    BLAZE_TIMER_STEP("FMOD misc inits 2");

    FMOD_CALL(m_system->getSoftwareFormat(&m_sampleRate, nullptr, nullptr));

    m_system->createChannelGroup(nullptr, &m_backgroundMusicChannel);
    m_backgroundMusicChannel->setVolumeRamp(false);
    m_backgroundMusicChannel->getDSP(-1, &m_mainDSP);
    m_mainDSP->setMeteringEnabled(false, true);

    m_system->createChannelGroup(nullptr, &m_globalChannel);
    m_system->createChannelGroup(nullptr, &m_channelGroup2);

    m_globalChannel->addGroup(m_channelGroup2, true, nullptr);

    FMOD::DSP* dsp = nullptr;
    m_system->createDSPByType(FMOD_DSP_TYPE_LIMITER, &dsp);
    dsp->setParameterFloat(1, 0.f);
    dsp->setParameterBool(3, true);

    FMOD_CALL(m_globalChannel->addDSP(1, dsp));

    dsp = nullptr;
    m_system->createDSPByType(FMOD_DSP_TYPE_LIMITER, &dsp);
    dsp->setParameterFloat(1, 0.f);
    dsp->setParameterBool(3, true);
    FMOD_CALL(m_backgroundMusicChannel->addDSP(1, dsp));

    dsp = nullptr;
    m_system->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &dsp);
    m_channelGroup2->addDSP(0, dsp);

    this->updateReverb(m_reverbPreset, true);

    m_globalChannel->getDSP(-1, &m_globalChannelDSP);
    m_globalChannelDSP->setMeteringEnabled(false, true);

    m_fields->initialized = true;
}