#include "ObjectLoader.hpp"
#include "objects.hpp"

using namespace geode::prelude;

// TODO: determine
static constexpr size_t MIN_OBJECTS_FOR_THREADING = 10000;
static constexpr size_t MIN_OBJECTS_PER_THREAD = 20000;
static constexpr size_t MAX_BATCH_OBJECTS = 256;
static constexpr size_t MAX_QUEUED_OBJECTS = MAX_BATCH_OBJECTS * 32;

namespace blaze {

static size_t optimalBatchSize(size_t totalObjects) {
    // TODO: determine best numbers
    if (totalObjects > 50000) {
        return MAX_BATCH_OBJECTS;
    } else if (totalObjects > 20000) {
        return MAX_BATCH_OBJECTS / 2;
    } else {
        return MAX_BATCH_OBJECTS / 4;
    }
}

template <typename F>
struct ExtensibleRange {
    ExtensibleRange(F onOverflow, size_t limit) : m_onOverflow(std::move(onOverflow)), m_limit(limit) {}

    ~ExtensibleRange() {
        this->flush();
    }

    void flush() {
        if (m_range) {
            m_onOverflow(*m_range);
            m_range = std::nullopt;
        }
    }

    void extend(size_t idx) {
        if (!m_range) {
            m_range = ObjectLoadChunk{idx, idx + 1};
        } else if (idx == m_range->m_end) {
            m_range->m_end++;
        } else {
            this->flush();
            m_range = ObjectLoadChunk{idx, idx + 1};
        }

        if (m_range->size() >= m_limit) {
            this->flush();
        }
    }

    size_t size() const {
        return m_range ? m_range->size() : 0;
    }

private:
    F m_onOverflow;
    size_t m_limit;
    std::optional<ObjectLoadChunk> m_range;
};

ObjectLoader::ObjectLoader(PlayLayer* playLayer) : m_playLayer(playLayer) {
    m_loadStart = asp::Instant::now();

    size_t toSkip = m_playLayer->m_objectsCreated;
    m_objects.reserve(m_playLayer->m_objectStrings.size() - toSkip);

    for (size_t i = toSkip; i < m_playLayer->m_objectStrings.size(); i++) {
        auto& s = m_playLayer->m_objectStrings[i];
        m_objects.emplace_back(&s, i - toSkip);
    }

    m_queueIndex = 0;
    m_loadState.m_gameLayer = m_playLayer;
    m_loadState.m_ldm = m_playLayer->m_level->m_lowDetailModeToggled;
    m_loadState.m_levelType = m_playLayer->m_level->m_levelType;
    m_loadState.m_uniqueIdBase = getUniqueGameObjectId();
    getUniqueGameObjectId() += m_objects.size();
    m_loadState.m_varianceSeed = getVarianceSeed();

    // enable multithreading if the object count is high enough
    if (m_objects.size() >= MIN_OBJECTS_FOR_THREADING) {
        // TODO: heuristic for initial count of threads
        uint32_t nThreads = m_objects.size() / MIN_OBJECTS_PER_THREAD;
        // but require at least 4 threads
        nThreads = std::max(nThreads, 4u);
        // but no more than available hardware threads
        nThreads = std::min(nThreads, std::thread::hardware_concurrency());

        for (size_t i = 0; i < nThreads; i++) {
            m_threads.emplace_back(std::thread(&ObjectLoader::workerFunc, this));
        }

        // pre-fill some scratch spaces for objects, use batch size / 2
        size_t batchSize = optimalBatchSize(m_objects.size());
        m_loadState.m_scratch.resize(batchSize / 2);
    }
}

void ObjectLoader::processBatch() {
    if (m_threads.empty()) {
        this->processBatchSync();
        return;
    }

    // process main thread things
    for (auto range : m_mainThreadQueue.drain()) {
        m_toDequeFromMain.fetch_sub(range.size(), std::memory_order::release);
        BLAZE_TRACE_NOISY("dequeued {} objects from main thread queue", range.size());

        ExtensibleRange curRange{[&](const ObjectLoadChunk& range) {
            m_workerQueue.push(range);
        }, MAX_BATCH_OBJECTS};

        for (size_t idx = range.m_start; idx < range.m_end; idx++) {
            auto& obj = m_objects[idx];
            if (obj.needsOrderedPolyInit() && obj.state() == GameObjectBuilder::State::BasicDone) {
                // polymorphic init is next, and we want it to happen in order for this object
                obj.setState(GameObjectBuilder::State::PolyInitMainPending);
                m_waitingForPolyInit++;
                continue;
            }

            if (this->advanceObjectMainThread(idx)) {
                continue;
            }

            // queue back to worker thread
            curRange.extend(idx);
        }
    }

    // enqueue objects until we are at limit or exhausted the vector
    size_t batchSize = optimalBatchSize(m_objects.size());

    while ((m_inFlight - m_waitingForPolyInit) < MAX_QUEUED_OBJECTS) {
        if (m_queueIndex >= m_objects.size()) {
            break;
        }

        ObjectLoadChunk range;
        range.m_start = m_queueIndex;
        range.m_end = std::min(m_queueIndex + batchSize, m_objects.size());
        m_queueIndex = range.m_end;
        m_inFlight += range.m_end - range.m_start;

        // enqueue this object
        m_workerQueue.push(range);

        BLAZE_TRACE_NOISY("pushed {} objects to workers", range.size());
    }

    // process polymorphic init on objects that require it ordered
    this->processPolyInitObjects();

    // add objects to playlayer
    this->addNextObjects();

    // some minor cleanup
    m_loadState.clearAllDirty();

    BLAZE_TRACE_NOISY("poly init count: {}, added count: {}, total objs: {}", m_polyInitCount, m_addedCount, m_objects.size());
}

bool ObjectLoader::advanceObjectMainThread(size_t idx) {
    GameObjectAdvanceResult result;
    do {
        result = m_objects[idx].advance(m_loadState, true);
    } while (result == GameObjectAdvanceResult::RequiresMainThread);

    if (result == GameObjectAdvanceResult::Finished) {
        // complete
        m_finishedCount.fetch_add(1, std::memory_order::release);
        m_inFlight--;

        this->onObjectFinished(idx);
        return true;
    }
    return false;
}

void ObjectLoader::processBatchSync() {
    // regular, main thread loading
    size_t objectsCreated = m_queueIndex;
    size_t totalObjects = m_objects.size();
    if (objectsCreated >= totalObjects) {
        return;
    }

    double batches = std::min(std::floor(totalObjects / 1000.0) + 10.0, 100.0);
    size_t objectsPerBatch = std::ceil(totalObjects / batches);

    size_t endIndex = std::min(objectsCreated + objectsPerBatch, totalObjects);

    for (size_t index = objectsCreated; index < endIndex; index++) {
        GameObjectAdvanceResult result;
        do {
            result = m_objects[index].advance(m_loadState, true);
        } while (result != GameObjectAdvanceResult::Finished);

        this->onObjectFinished(index);
    }

    m_queueIndex = endIndex;

    this->addNextObjects();
}

void ObjectLoader::onObjectFinished(size_t idx) {
    // nothing for now
}

void ObjectLoader::processPolyInitObjects() {
    ExtensibleRange qrange{[&](const ObjectLoadChunk& range) {
        m_workerQueue.push(range);
    }, MAX_BATCH_OBJECTS};

    // process in level string order
    while (m_polyInitCount < m_objects.size()) {
        auto& obj = m_objects[m_polyInitCount];
        auto state = obj.state();

        // if this object is still in the early stages, end processing
        if ((int)state < (int)GameObjectBuilder::State::PolyInitMainPending) {
            break;
        }

        // if this object already did poly init, move on to the next one
        if ((int)state >= (int)GameObjectBuilder::State::PolyInitDone) {
            m_polyInitCount++;
            continue;
        }

        // otherwise, this object is next in line for the polymorphic init!
        auto idx = m_polyInitCount++;
        m_waitingForPolyInit--;
        if (this->advanceObjectMainThread(idx)) {
            continue;
        }

        // incomplete, queue back to worker thread
        qrange.extend(idx);
    }

    qrange.flush();
}

void ObjectLoader::addNextObjects() {
    // add all objects in the same order as they appear in the level string
    while (m_addedCount < m_objects.size()) {
        auto& obj = m_objects[m_addedCount];
        if (!obj.completed()) {
            break;
        }

        if (obj.state() == GameObjectBuilder::State::Skipped) {
            obj.takeOutObject(); // autorelease and discard
            m_addedCount++;
            continue;
        }

        // this object is complete
        auto gobj = obj.takeOutObject();
        B_DEBUG_ASSERT(gobj && "object unavailable");

        this->addObject(gobj);
        m_addedCount++;
    }
}

// Perform steps here where order between objects is crucial
void ObjectLoader::addObject(GameObject* obj) {
    auto pl = m_loadState.m_gameLayer;

    obj->saveActiveColors();

    if (obj->m_objectID == 142) {
        reinterpret_cast<EnhancedGameObject*>(obj)->updateUserCoin();
    }

    if (obj->m_mainColorKeyIndex > 0) {
        if (obj->m_mainColorKeyIndex > pl->m_nextColorKey) {
            pl->m_nextColorKey = obj->m_mainColorKeyIndex;
        }
        if (obj->m_detailColorKeyIndex > pl->m_nextColorKey) {
            pl->m_nextColorKey = obj->m_detailColorKeyIndex;
        }
    } else {
        // allocate color channel for each active color key and store back in the object
        int colorCount = obj->hasSecondaryColor() ? 2 : 1;
        for (int i = 0; i < colorCount; i++) {
            auto colorKey = obj->getColorKey(i == 0, false);

            int colorIndex;
            if (auto* existing = pl->m_colorKeyDict->objectForKey(colorKey)) {
                colorIndex = static_cast<CCInteger*>(existing)->m_nValue;
            } else {
                colorIndex = pl->m_nextColorKey;
                pl->m_colorKeyDict->setObject(CCInteger::create(colorIndex), colorKey);
                pl->m_nextColorKey++;
            }

            if (i == 0) {
                obj->m_mainColorKeyIndex = colorIndex;
            } else {
                obj->m_detailColorKeyIndex = colorIndex;
            }
        }
    }

    // i hate this
    obj->m_varianceIndex = nextVarianceValue(m_loadState.m_varianceSeed);

    // only first three use coins are reigstered
    if (obj->getType() == GameObjectType::UserCoin) {
        if (pl->m_coinArray->count() >= 3) {
            return;
        }
        pl->m_coinArray->addObject(obj);
    }

    auto before = pl->m_objects->count();
    getUniqueGameObjectId() = obj->m_uniqueID + 1; // see comment in loadBasicProperties
    pl->addObject(obj);
    size_t advanceBy = pl->m_objects->count() - before - 1;
    // basically, addObject may create extra objects which messes up index of the variance value so we need to fast-forward it
    for (size_t i = 0; i < advanceBy; i++) {
        nextVarianceValue(m_loadState.m_varianceSeed);
    }
}

ObjectLoaderProgress ObjectLoader::progress() {
    if (m_threads.empty()) {
        return {m_queueIndex, m_objects.size()};
    } else {
        return {m_finishedCount.load(std::memory_order::acquire), m_objects.size()};
    }
}

void ObjectLoader::complete() {
    B_ASSERT(this->progress().complete() && "not all objects finished loading during complete");
    B_ASSERT_EQ(m_addedCount, m_objects.size(), "not all objects went to addObject during complete");

    size_t totalObjects = m_objects.size();

    this->terminateThreads();
    m_objects.clear();
    getVarianceSeed() = m_loadState.m_varianceSeed;

#ifdef BLAZE_DEBUG
    auto took = m_loadStart.elapsed();
    log::info("Took {} to load the level ({} objects)!", took, totalObjects);
#endif
}

void ObjectLoader::sleep(asp::Duration workTookTime) {
    if (m_threads.empty()) {
        // no sleep for sync loading
        return;
    }

    // if we blocked the thread for too long, no need to sleep, only sleep when the main thread actually had no work to do.
    if (workTookTime.millis() > 5) {
        return;
    }

    if (this->shouldMainSleep()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

bool ObjectLoader::shouldMainSleep() {
    return m_toDequeFromMain.load(std::memory_order::acquire) == 0;
}

void ObjectLoader::workerFunc() {
    while (!m_finished.load(std::memory_order::acquire)) {
        auto range = m_workerQueue.pop();

        if (range == ObjectLoadChunk{}) {
            // nothing to do, we are likely terminating
            break;
        }

        for (size_t i = range.m_start; i < range.m_end; i++) {
            GameObjectAdvanceResult result;
            do {
                result = m_objects[i].advance(m_loadState, false);
            } while (result == GameObjectAdvanceResult::Pending);

            // finished || requires main thread, queue that object
        }

        m_toDequeFromMain.fetch_add(range.size(), std::memory_order::release);
        m_mainThreadQueue.push(range);
    }
}

void ObjectLoader::terminateThreads() {
    m_finished.store(true, std::memory_order::release);

    for (size_t i = 0; i < m_threads.size(); i++) {
        m_workerQueue.push(ObjectLoadChunk{});
    }

    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    m_threads.clear();
}

}
