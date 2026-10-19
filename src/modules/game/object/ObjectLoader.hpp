#pragma once
#include <Geode/Geode.hpp>
#include <asp/sync/Channel.hpp>
#include "GameObjectBuilder.hpp"

namespace blaze {

static constexpr size_t MAX_SCRATCHES = 1024; // at most 24mb of non-compact storage

struct ObjectsLoadState  {
    PlayLayer* m_gameLayer = nullptr;
    GJLevelType m_levelType;
    int m_uniqueIdBase = 0;
    uint64_t m_varianceSeed;
    bool m_ldm = false;

    std::vector<ObjectProps<false>> m_scratch;
    std::vector<ObjectProps<false>> m_dirtyScratch;

    ObjectProps<false> getScratch(const ObjectProps<true>& other) {
        auto out = getOrCreateScratch();
        other.fillNonCompact(out);
        return out;
    }

    void returnScratch(ObjectProps<false> scratch) {
        if (m_scratch.size() + m_dirtyScratch.size() >= MAX_SCRATCHES) {
            // do nothing, just destroy this scratch
            return;
        }

        m_dirtyScratch.push_back(std::move(scratch));
    }

    void clearAllDirty() {
        for (auto& scratch : m_dirtyScratch) {
            scratch.reset();
        }
        m_scratch.insert(
            m_scratch.end(),
            std::make_move_iterator(m_dirtyScratch.begin()),
            std::make_move_iterator(m_dirtyScratch.end())
        );
        m_dirtyScratch.clear();
    }

private:
    ObjectProps<false> getOrCreateScratch() {
        if (!m_scratch.empty()) {
            auto out = std::move(m_scratch.back());
            m_scratch.pop_back();
            return out;
        }

        if (!m_dirtyScratch.empty()) {
            auto out = std::move(m_dirtyScratch.back());
            out.reset();
            m_dirtyScratch.pop_back();
            return out;
        }

        return {};
    }
};

struct ObjectLoadChunk {
    size_t m_start, m_end;

    size_t size() const {
        return m_end - m_start;
    }

    bool operator==(const ObjectLoadChunk& other) const = default;
};

struct ObjectLoaderProgress {
    size_t loadedObjects;
    size_t totalObjects;

    float percent() const {
        if (totalObjects == 0) return 1.0f;
        return (float)loadedObjects / (float)totalObjects;
    }

    bool complete() const {
        return loadedObjects >= totalObjects;
    }
};

class ObjectLoader {
public:
    ObjectLoader(PlayLayer* playLayer);

    void processBatch();
    void complete();
    void sleep(asp::Duration workTookTime);

    ObjectLoaderProgress progress();

private:
    std::vector<GameObjectBuilder> m_objects;
    ObjectsLoadState m_loadState;
    PlayLayer* m_playLayer;
    asp::Instant m_loadStart;

    // async loading
    std::vector<std::thread> m_threads;
    asp::Channel<ObjectLoadChunk> m_workerQueue;
    asp::Channel<ObjectLoadChunk> m_mainThreadQueue;
    std::atomic<size_t> m_finishedCount;
    std::atomic<bool> m_finished{false};
    std::atomic<size_t> m_toDequeFromMain{0};
    size_t m_inFlight = 0;
    size_t m_queueIndex = 0;
    size_t m_polyInitCount = 0;
    size_t m_addedCount = 0;
    size_t m_waitingForPolyInit = 0;

    void workerFunc();
    void terminateThreads();

    void processBatchSync();

    bool shouldMainSleep();

    /// Returns if object is complete
    bool advanceObjectMainThread(size_t idx);
    void onObjectFinished(size_t idx);
    void processPolyInitObjects();
    void addNextObjects();
    void addObject(GameObject* object);

};

}

// TODO: quick ldm skip, quick object id check
