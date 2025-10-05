#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <asp/time/Instant.hpp>
#include <asp/sync/SpinLock.hpp>
#include <asp/sync/Notify.hpp>
#include <asp/thread.hpp>
#include <asp/iter.hpp>
#include <util.hpp>

#ifdef ASP_IS_X86
# include <immintrin.h>
#else
# include <arm_acle.h>
#endif

static void microYield() {
#ifdef ASP_IS_X86
    _mm_pause();
#else
    __yield();
#endif
}

using namespace geode::prelude;
using namespace asp::time;

namespace blaze {

static asp::SpinLock<int> g_groupLock;
static asp::SpinLock<int> g_raLock;
static asp::SpinLock<int> g_maLock;
static asp::SpinLock<int> g_sectionLock;
static asp::SpinLock<int> g_testLock;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
constexpr auto OBJECT_LOCK_OFFSET = offsetof(GameObject, m_detailUsesHSV) + 1;
#pragma clang diagnostic pop

static ASP_FORCE_INLINE std::atomic_uint8_t& getObjectLock(GameObject* obj) {
    // use padding byte after m_detailUsesHSV as a mutex byte
    return *reinterpret_cast<std::atomic_uint8_t*>(
        reinterpret_cast<uint8_t*>(obj) + OBJECT_LOCK_OFFSET
    );
}

struct ObjectLockGuard {
    GameObject* obj;

    ASP_FORCE_INLINE ObjectLockGuard(GameObject* obj) : obj(obj) {
        asp::acquireAtomicLock(reinterpret_cast<volatile uint8_t*>(&getObjectLock(obj)));
    }

    ASP_FORCE_INLINE ~ObjectLockGuard() {
        asp::releaseAtomicLock(reinterpret_cast<volatile uint8_t*>(&getObjectLock(obj)));
    }
};

struct ThreadPool {
public:
    using Task = std::function<void()>;

    ThreadPool(size_t threads = std::thread::hardware_concurrency()) : m_storage(std::make_shared<Storage>()) {
        auto workers = std::make_unique<Worker[]>(threads);
        m_storage->workers = std::move(workers);
        m_storage->workerCount = threads;

        for (size_t i = 0; i < threads; i++) {
            std::thread thread([storage = m_storage, i] {
                auto& worker = storage->workers[i];
                auto& tasks = storage->tasks;
                auto& notify = storage->notify;

                while (true) {
                    auto queue = tasks.lock();
                    if (queue->empty()) {
                        queue.unlock();
                        notify.wait();
                        continue;
                    }

                    auto task = std::move(queue->front());
                    queue->pop_front();
                    queue.unlock();

                    worker.working.store(true, std::memory_order_relaxed);
                    task();
                    worker.working.store(false, std::memory_order_relaxed);
                }
            });

            new (&m_storage->workers[i]) Worker{
                .thread = std::move(thread),
                .working = false
            };
        }
    }

    ~ThreadPool() {
        m_destructing = true;
        this->join();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    void pushTask(const Task& task) {
        m_storage->tasks.lock()->push_back(task);
        m_storage->notify.notifyOne();
    }

    void pushTask(Task&& task) {
        m_storage->tasks.lock()->push_back(std::move(task));
        m_storage->notify.notifyOne();
    }

    void join() {
        while (!m_storage->tasks.lock()->empty()) {
            // if we are destructing, it's possible that all threads are already dead, just terminate
            if (m_destructing) {
                return;
            }

            microYield();
        }

        // wait for all workers to finish
        bool stillWorking;

        do {
            if (m_destructing) {
                return;
            }

            microYield();

            stillWorking = false;
            for (size_t i = 0; i < m_storage->workerCount; i++) {
                if (m_storage->workers[i].working.load(std::memory_order_relaxed)) {
                    stillWorking = true;
                    break;
                }
            }
        } while (stillWorking);
    }

private:
    struct Worker {
        std::thread thread;
        std::atomic_bool working = false;
    };

    struct Storage {
        std::unique_ptr<Worker[]> workers;
        size_t workerCount = 0;
        asp::SpinLock<std::deque<Task>> tasks;
        asp::Notify notify;
    };

    std::shared_ptr<Storage> m_storage;
    bool m_destructing = false;
};

class $modify(ParallelGJBGL, GJBaseGameLayer) {
    struct Fields {
        std::optional<ThreadPool> m_threadPool;
    };

    ThreadPool& getPool() {
        auto& fields = *m_fields.self();

        if (!fields.m_threadPool) {
            fields.m_threadPool.emplace();
        }

        return fields.m_threadPool.value();
    }

    $override
    void processMoveActionsStep(float p0, bool p1) {
        if (m_isEditor) {
            return GJBaseGameLayer::processMoveActionsStep(p0, p1);
        }

        // log::debug("Processing move actions step with p0 = {}, p1 = {}", p0, p1);

        m_unked0 = 0;
        m_disabledObjectsCount = 0;
        m_unked8 = 0;
        m_areaObjectsCount = 0;

        for (auto& act : m_gameState.m_dynamicObjActions1) {
            int gid = static_cast<EffectGameObject*>(act.m_gameObject1)->m_targetGroupID;
            auto command = m_effectManager->tryGetMoveCommandNode(gid);
            if (command) command->m_unk0d1 = true;
        }

        this->processDynamicObjectActions(1, p0);
        this->processTransformActions(p1);
        this->processRotationActionsReimpl();
        this->processDynamicObjectActions(0, p0);
        this->processMoveActionsReimpl();
        this->processPlayerFollowActions(p0);
        this->processAdvancedFollowActions(p0);
        this->processFollowActions();
        this->processAreaActions(p0, p1);

        if (m_isEditor && m_disabledObjectsCount > 0) {
            for (int i = 0; i < m_disabledObjectsCount; i++) {
                auto obj = m_disabledObjects[i];
                obj->quickUpdatePosition();
            }
        }
    }

    // -- Below: rotation reimpl --

    void processRotationActionsReimpl() {
        auto p1 = Instant::now();

        float sx = m_objectLayer->getScaleX();
        float sy = m_objectLayer->getScaleY();
        auto pos = m_objectLayer->getPosition();

        m_objectLayer->setScale(1.f);
        m_objectLayer->setPosition({0.f, 0.f});

        for (auto& pair : m_effectManager->m_unkMap5c8) {
            for (auto obj : pair.second) {
                obj->m_unkInt204 = m_gameState.m_unkUint2;
            }
        }

        size_t totalToRotate = asp::iter::from(m_effectManager->m_unkVector5b0)
            .filterMap([&](auto obj) -> std::optional<size_t> {
                if (obj->m_unkInt204 != m_gameState.m_unkUint2 || obj->m_someInterpValue1RelatedFalse) {
                    return std::nullopt;
                }

                auto sgroup = this->getStaticGroup(obj->m_targetGroupID);
                auto ogroup = this->getOptimizedGroup(obj->m_targetGroupID);

                if (sgroup->data->num) {
                    ogroup = this->getGroup(obj->m_targetGroupID);
                }

                return ogroup->data->num;
            })
            .sum();

        bool shouldParallelize = totalToRotate > 1000;
        // log::debug("Rotating {} objects in total (parallel: {})", totalToRotate, shouldParallelize);

        auto p2 = Instant::now();

        if (shouldParallelize) {
            this->customPerformRotationsParallel();
        } else {
            this->customPerformRotations();
        }

        m_objectLayer->setScaleX(sx);
        m_objectLayer->setScaleY(sy);
        m_objectLayer->setPosition(pos);

        auto p3 = Instant::now();

        // log::debug(
        //     "Rotated {} objects (overhead: {}, rotating: {})",
        //     totalToRotate,
        //     p2.durationSince(p1).toString(),
        //     p3.durationSince(p2).toString()
        // );
    }

    void customPerformRotations() {
        for (auto& obj : m_effectManager->m_unkVector5b0) {
            if (obj->m_unkInt204 != m_gameState.m_unkUint2 || obj->m_someInterpValue1RelatedFalse) {
                continue;
            }

            auto mainObj = this->tryGetMainObject(obj->m_centerGroupID);
            auto sgroup = this->getStaticGroup(obj->m_targetGroupID);
            auto ogroup = this->getOptimizedGroup(obj->m_targetGroupID);

            bool v17;
            double v18;

            if (sgroup->data->num) {
                v17 = false;
                ogroup = this->getGroup(obj->m_targetGroupID);
                v18 = obj->m_someInterpValue1RelatedOne - obj->m_someInterpValue1RelatedZero;
            } else {
                v17 = true;
                v18 = obj->m_someInterpValue2RelatedOne - obj->m_someInterpValue2RelatedZero;
            }
            float v46 = v18;

            bool finishRelated = obj->m_finishRelated;
            if (v18 == 0.0 && !finishRelated) {
                continue;
            }

            obj->m_someInterpValue1RelatedFalse = true;
            if (obj->m_lockObjectRotation) {
                v18 = 0.0;
            }

            float v47 = v18;
            this->claimRotationAction(obj->m_targetGroupID, obj->m_centerGroupID, v46, v47, v17, true);

            if (mainObj) {
                auto pos = mainObj->getUnmodifiedPosition();
                auto claimed = this->claimMoveAction(obj->m_targetGroupID, v17);

                m_areaTransformNode->setPosition(pos);
                m_areaTransformNode->setScaleX(1.0f);
                m_areaTransformNode->setScaleY(1.0f);
                m_areaScaleNode->setScaleX(1.0f);
                m_areaScaleNode->setScaleY(1.0f);
                m_areaTransformNode->setSkewX(0.f);
                m_areaTransformNode->setSkewY(0.f);
                m_areaSkewNode->setSkewX(0.f);
                m_areaSkewNode->setSkewY(0.f);
                m_areaTransformNode->setRotation(v46);
                this->prepareTransformParent(false);
                auto t1 = m_areaTransformNode->nodeToWorldTransform();
                auto t2 = m_areaSkewNode->nodeToWorldTransform();
                auto t3 = m_areaScaleNode->nodeToWorldTransform();
                m_areaTransformNode2->setScaleX(1.0f);
                m_areaTransformNode2->setScaleY(1.0f);
                m_areaTransformNode2->setSkewX(0.f);
                m_areaTransformNode2->setSkewY(0.f);
                m_rotatedCount += ogroup->data->num;

                for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                    obj->m_unk4fb = finishRelated;

                    if (!obj->m_isDecoration2) {
                        if (obj->m_unk4C4 != m_gameState.m_unkUint2) {
                            obj->m_lastPosition.x = obj->m_positionX;
                            obj->m_lastPosition.y = obj->m_positionY;
                            obj->m_unk4C4 = m_gameState.m_unkUint2;
                            obj->dirtifyObjectRect();
                        }
                    }

                    float xo = obj->m_positionXOffset;
                    float yo = obj->m_positionYOffset;
                    obj->m_isDirty = true;
                    obj->m_isUnmodifiedPosDirty = true;
                    float v32 = obj->m_positionX - xo - pos.x;
                    float v33 = obj->m_positionY - yo - pos.y;
                    auto tresult = __CCPointApplyAffineTransform({v32, v33}, t1);

                    obj->m_positionX = obj->m_positionXOffset + tresult.x + claimed.x;
                    obj->m_positionY = obj->m_positionYOffset + tresult.y + claimed.y;

                    if (v47 != 0.0 && obj->m_canRotateFree) {
                        obj->m_rotationXOffset += v47;
                        obj->m_rotationYOffset += v47;
                        this->customObjAddRotation(obj, v47);
                        if (obj->m_objectType != GameObjectType::Decoration && !obj->m_shouldUseOuterOb) {
                            obj->calculateOrientedBox();
                        }
                    }
                    this->updateObjectSection(obj);
                }

                this->updateDisabledObjectsLastPos(ogroup);
            } else if (ogroup && v47 != 0.0 && ogroup->data->num) {
                m_rotatedCount += ogroup->data->num;

                for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                    if (obj->m_canRotateFree) {
                        obj->m_rotationXOffset += v47;
                        obj->m_rotationYOffset += v47;
                        this->customObjAddRotation(obj, v47);
                        if (obj->m_objectType != GameObjectType::Decoration) {
                            obj->m_isObjectRectDirty = true;
                            obj->m_isOrientedBoxDirty = true;
                            if (!obj->m_shouldUseOuterOb) {
                                obj->calculateOrientedBox();
                            }
                        }
                    }
                }
            }
        }
    }

    void customPerformRotationsParallel() {
        auto& pool = this->getPool();

        for (auto obj : m_effectManager->m_unkVector5b0) {
            if (obj->m_unkInt204 != m_gameState.m_unkUint2 || obj->m_someInterpValue1RelatedFalse) {
                continue;
            }

            pool.pushTask([this, obj] {
                auto mainObj = this->tryGetMainObject(obj->m_centerGroupID);
                auto sgroup = this->customGetStaticGroup(obj->m_targetGroupID);
                auto ogroup = this->customGetOptimizedGroup(obj->m_targetGroupID);

                bool v17;
                double v18;

                if (sgroup->data->num) {
                    v17 = false;
                    ogroup = this->customGetGroup(obj->m_targetGroupID);
                    v18 = obj->m_someInterpValue1RelatedOne - obj->m_someInterpValue1RelatedZero;
                } else {
                    v17 = true;
                    v18 = obj->m_someInterpValue2RelatedOne - obj->m_someInterpValue2RelatedZero;
                }
                float v46 = v18;

                bool finishRelated = obj->m_finishRelated;
                if (v18 == 0.0 && !finishRelated) {
                    return;
                }

                obj->m_someInterpValue1RelatedFalse = true;
                if (obj->m_lockObjectRotation) {
                    v18 = 0.0;
                }

                float v47 = v18;
                {
                    auto _g = g_raLock.lock();
                    this->claimRotationAction(obj->m_targetGroupID, obj->m_centerGroupID, v46, v47, v17, true);
                }

                if (mainObj) {
                    auto _lock = g_maLock.lock();

                    auto pos = mainObj->getUnmodifiedPosition();
                    auto claimed = this->claimMoveAction(obj->m_targetGroupID, v17);

                    m_areaTransformNode->setPosition(pos);
                    m_areaTransformNode->setScaleX(1.0f);
                    m_areaTransformNode->setScaleY(1.0f);
                    m_areaScaleNode->setScaleX(1.0f);
                    m_areaScaleNode->setScaleY(1.0f);
                    m_areaTransformNode->setSkewX(0.f);
                    m_areaTransformNode->setSkewY(0.f);
                    m_areaSkewNode->setSkewX(0.f);
                    m_areaSkewNode->setSkewY(0.f);
                    m_areaTransformNode->setRotation(v46);
                    this->prepareTransformParent(false);
                    auto t1 = m_areaTransformNode->nodeToWorldTransform();
                    auto t2 = m_areaSkewNode->nodeToWorldTransform();
                    auto t3 = m_areaScaleNode->nodeToWorldTransform();
                    m_areaTransformNode2->setScaleX(1.0f);
                    m_areaTransformNode2->setScaleY(1.0f);
                    m_areaTransformNode2->setSkewX(0.f);
                    m_areaTransformNode2->setSkewY(0.f);
                    m_rotatedCount += ogroup->data->num;

                    _lock.unlock();

                    for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                        ObjectLockGuard _guard(obj);

                        obj->m_unk4fb = finishRelated;

                        if (!obj->m_isDecoration2) {
                            if (obj->m_unk4C4 != m_gameState.m_unkUint2) {
                                obj->m_lastPosition.x = obj->m_positionX;
                                obj->m_lastPosition.y = obj->m_positionY;
                                obj->m_unk4C4 = m_gameState.m_unkUint2;
                                obj->dirtifyObjectRect();
                            }
                        }

                        float xo = obj->m_positionXOffset;
                        float yo = obj->m_positionYOffset;
                        obj->m_isDirty = true;
                        obj->m_isUnmodifiedPosDirty = true;
                        float v32 = obj->m_positionX - xo - pos.x;
                        float v33 = obj->m_positionY - yo - pos.y;
                        auto tresult = __CCPointApplyAffineTransform({v32, v33}, t1);

                        obj->m_positionX = obj->m_positionXOffset + tresult.x + claimed.x;
                        obj->m_positionY = obj->m_positionYOffset + tresult.y + claimed.y;

                        if (v47 != 0.0 && obj->m_canRotateFree) {
                            obj->m_rotationXOffset += v47;
                            obj->m_rotationYOffset += v47;
                            this->customObjAddRotation(obj, v47);
                            if (obj->m_objectType != GameObjectType::Decoration && !obj->m_shouldUseOuterOb) {
                                obj->calculateOrientedBox();
                            }
                        }
                        this->updateObjectSectionReimpl(obj);
                    }

                    this->updateDisabledObjectsLastPos(ogroup);
                } else if (ogroup && v47 != 0.0 && ogroup->data->num) {
                    m_rotatedCount += ogroup->data->num;

                    for (auto obj : CCArrayExt<GameObject>(ogroup)) {
                        if (obj->m_canRotateFree) {
                            obj->m_rotationXOffset += v47;
                            obj->m_rotationYOffset += v47;
                            this->customObjAddRotation(obj, v47);
                            if (obj->m_objectType != GameObjectType::Decoration) {
                                obj->m_isObjectRectDirty = true;
                                obj->m_isOrientedBoxDirty = true;
                                if (!obj->m_shouldUseOuterOb) {
                                    obj->calculateOrientedBox();
                                }
                            }
                        }
                    }
                }
            });
        }

        pool.join();
    }

    void customObjAddRotation(GameObject* obj, float r) {
        if (r == 0.0) return;

        // funny hack
        bool prd = obj->m_bRecursiveDirty;
        obj->m_bRecursiveDirty = true;

        if (obj->m_fRotationX == obj->m_fRotationY) {
            obj->setRotation(obj->m_fRotationX + r);
        } else {
            obj->setRotationX(obj->m_fRotationX + r);
            obj->setRotationY(obj->m_fRotationY + r);
        }

        obj->m_bRecursiveDirty = prd;
    }

    // -- Below: thread safe group reimpls --

    CCArray* customGetGroupWith(int id, gd::vector<CCArray*>& vec, CCDictionary* dict) {
        id = std::clamp(id, 0, 9999);
        auto group = vec[id]; // loads are atomic on most platforms so this is fine
        if (group) return group;

        auto _lock = g_groupLock.lock();
        group = vec[id]; // access again in case another thread just created it
        if (!group) {
            group = cocos2d::CCArray::create();
            dict->setObject(group, id);
            vec[id] = group;
        }

        return group;
    }

    CCArray* customGetStaticGroup(int id) {
        return this->customGetGroupWith(id, m_staticGroups, m_staticGroupDict);
    }

    CCArray* customGetOptimizedGroup(int id) {
        return this->customGetGroupWith(id, m_optimizedGroups, m_optimizedGroupDict);
    }

    CCArray* customGetGroup(int id) {
        return this->customGetGroupWith(id, m_groups, m_groupDict);
    }

    // -- Below: move reimpl --

    void processMoveActionsReimpl() {
        auto& vec = m_effectManager->m_unkVector708;

        // inline processMoveCalculations
        for (auto& action : vec) {
            float val = -action.m_gameObject->m_rotationXOffset * M_PI / 180.f;
            float cosine = cosf(val);
            float sine = sinf(val);
            float offx = action.m_offset.x;
            float offy = action.m_offset.y;

            CCPoint point;
            point.x = (offx * cosine - offy * sine) * (action.m_gameObject->m_scaleXOffset + 1.0f);
            point.y = (offy * cosine + offx * sine) * (action.m_gameObject->m_scaleYOffset + 1.0f);
            point -= action.m_offset;

            auto mn = action.m_moveNode;
            mn->m_unk0d1 = false;
            mn->m_unk038 += point.x;
            mn->m_unk040 += point.y;
            mn->m_unk090 += point.x;
            mn->m_unk098 += point.y;
        }
        vec.clear();

        struct ToProcess {
            CCMoveCNode* moveNode;
            CCArray* group;
            double offsetx;
            double offsety;
        };

        static std::vector<ToProcess> processLocally;
        size_t processingLocallyTotal = 0;
        bool anyInPool = false;
        processLocally.clear();

        auto& pool = this->getPool();

        auto enqueue = [&](CCMoveCNode* action, CCArray* group, double offsetx, double offsety) {
            size_t count = group->data->num;
            if (count == 0) return;

            if (count < 50 && processingLocallyTotal < 500) {
                processLocally.push_back({action, group, offsetx, offsety});
                processingLocallyTotal += count;
            } else if (count < 1000) {
                // push to the thread pool directly
                pool.pushTask([this, action, group, offsetx, offsety] {
                    this->moveObjectsReimpl<true>(group, offsetx, offsety, action->m_eObjType == CCObjectType::GameObject);
                });
                anyInPool = true;
            } else {
                // huge amount of objects, split into chunks
                size_t chunkSize = 500;
                size_t chunks = (count + chunkSize - 1) / chunkSize;

                for (size_t i = 0; i < chunks; i++) {
                    size_t startIdx = i * chunkSize;
                    size_t thisChunkSize = std::min(chunkSize, count - startIdx);

                    pool.pushTask([this, action, group, startIdx, thisChunkSize, offsetx, offsety] {
                        this->moveObjectsPartialCustom<true>(group, startIdx, thisChunkSize, offsetx, offsety, action->m_eObjType == CCObjectType::GameObject);
                    });
                }
                anyInPool = true;

                // TODO: ideally we also want to call updateDisabledObjectsLastPos after all chunks are done,
                // but this is unused in PlayLayer, and we don't support editor right now.
            }
        };

        for (auto action : m_effectManager->m_unkVector6c0) {
            if (action->m_unk0d0) continue;

            int tag = action->getTag();
            auto sgroup = this->customGetStaticGroup(tag);
            if (sgroup && (action->m_unk038 != 0.0 || action->m_unk040 != 0.0 || action->m_unk078)) {
                enqueue(action, sgroup, action->m_unk038, action->m_unk040);
            }

            sgroup = this->customGetOptimizedGroup(tag);
            if (sgroup && (action->m_unk090 != 0.0 || action->m_unk098 != 0.0 || action->m_unk078)) {
                enqueue(action, sgroup, action->m_unk090, action->m_unk098);
            }
        }

        // process small stuff here
        for (auto& item : processLocally) {
            // don't do any locking if everything was done on main
            if (anyInPool) {
                this->moveObjectsReimpl<true>(
                    item.group,
                    item.offsetx,
                    item.offsety,
                    item.moveNode->m_eObjType == CCObjectType::GameObject
                );
            } else {
                this->moveObjectsReimpl<false>(
                    item.group,
                    item.offsetx,
                    item.offsety,
                    item.moveNode->m_eObjType == CCObjectType::GameObject
                );
            }
        }

        // wait for the rest
        pool.join();
    }

    template <bool DoLock = true>
    void moveObjectsReimpl(CCArray* arr, double offsetx, double offsety, bool p3) {
        int count = arr->data->num;
        if (count == 0) return;

        this->moveObjectsPartialCustom<DoLock>(arr, 0, count, offsetx, offsety, p3);
        this->updateDisabledObjectsLastPos(arr);
    }

    template <bool DoLock>
    void moveObjectsPartialCustom(CCArray* arr, size_t startIdx, size_t count, double offsetx, double offsety, bool p3) {
        // log::debug("Moving {} objects with p1 = {}, p2 = {}, p3 = {}", count, offsetx, offsety, p3);

        reinterpret_cast<std::atomic_int*>(&m_movedCount)->fetch_add(count, std::memory_order_relaxed);

        for (size_t i = startIdx; i < startIdx + count; i++) {
            auto obj = static_cast<GameObject*>(arr->data->arr[i]);

            std::conditional_t<DoLock, ObjectLockGuard, void*> _guard{obj};

            if (!obj->m_isDecoration2 && obj->m_unk4C4 != m_gameState.m_unkUint2) {
                obj->m_lastPosition.x = obj->m_positionX;
                obj->m_lastPosition.y = obj->m_positionY;
                obj->m_unk4C4 = m_gameState.m_unkUint2;
                obj->dirtifyObjectRect();
            }

            if (offsetx != 0.0 && !obj->m_tempOffsetXRelated) {
                obj->m_positionX += offsetx;
            }

            if (offsety != 0.0) {
                obj->m_positionY += offsety;
            }

            obj->dirtifyObjectPos();

            this->updateObjectSectionReimpl(obj);
        }
    }

    template <bool DoLock = true>
    void updateObjectSectionReimpl(GameObject* obj) {
        if (obj->m_outerSectionIndex < 0) return;

        double posx = obj->m_positionX;
        if (posx <= 0.0) {
            posx = 0.0;
        } else {
            if (posx < 1000000.0) {
                posx = (double)m_sectionXFactor * posx;
            } else {
                posx = (double)m_sectionXFactor * 1000000.0;
            }
        }

        double posy = obj->m_positionY;
        if (posy <= 0.0) {
            posy = 0.0;
        } else {
            if (posy < 1000000.0) {
                posy = (double)m_sectionYFactor * posy;
            } else {
                posy = (double)m_sectionYFactor * 1000000.0;
            }
        }

        if ((int)posx != obj->m_outerSectionIndex || (int)posy != obj->m_middleSectionIndex) {
            auto _lock = this->_lockSections<DoLock>();

            // inline reorderObjectSection (2 calls)
            this->removeObjectFromSection(obj);
            this->addToSection(obj);

            if (obj->m_isActivated) {
                int outer = obj->m_outerSectionIndex;
                int middle = obj->m_middleSectionIndex;

                if (outer > m_rightSectionIndex || outer < m_leftSectionIndex || middle < m_bottomSectionIndex || middle > m_topSectionIndex) {
                    m_objectsToDeactivate->setObject(obj, obj->m_uniqueID);
                    obj->m_unk3ee = true;
                }
            }
        }
    }

    template <bool Lock>
    auto _lockSections();

    template <>
    auto _lockSections<true>() {
        return g_sectionLock.lock();
    }

    template <>
    auto _lockSections<false>() {
        return 0;
    }
};

class $modify(PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        auto gjbgl = reinterpret_cast<ParallelGJBGL*>(this);
        for (auto obj : CCArrayExt<GameObject>(gjbgl->m_objects)) {
            // initialize locks
            getObjectLock(obj).store(0, std::memory_order_relaxed);
        }
    }
};

}
