#pragma once
#include "ObjectProps.hpp"

namespace blaze {

enum class GameObjectAdvanceResult {
    /// Still pending, run again on worker thread
    Pending,
    /// Requires main thread for next step
    RequiresMainThread,
    /// Completed, errored or skipped
    Finished,
};

struct ObjectsLoadState;

/// Small state machine for a GameObject being built
class GameObjectBuilder {
public:
    GameObjectBuilder(std::string* source, uint32_t index);
    ~GameObjectBuilder();

    GameObjectBuilder(const GameObjectBuilder&) = delete;
    GameObjectBuilder& operator=(const GameObjectBuilder&) = delete;
    GameObjectBuilder(GameObjectBuilder&& other) noexcept;
    GameObjectBuilder& operator=(GameObjectBuilder&&) noexcept;

    enum class State : uint8_t {
        /// Nothing has been done yet
        Initial,
        /// Object properties have been parsed, it's time to create the object
        Parsed,
        /// Created: GameObject has been created, now initialization remains
        Created,
        /// Basic initialization is done, some next steps require main thread work
        BasicDone,
        /// Polymorphic init is pending and order matters
        PolyInitMainPending,
        /// Polymorphic init is done, more properties remain
        PolyInitDone,
        /// All properties loaded, only post-init steps remain that require main thread.
        AllPropsDone,

        /// Creation skipped - this object is not needed (ldm) or cannot be loaded (unknown object ID)
        Skipped,
        /// Everything is done, the object was added to PlayLayer.
        Finished,
    };

    State state() const;
    void setState(State state);

    /// Returns if state is either skipped or finished
    bool completed() const;

    // advance the state machine forward
    GameObjectAdvanceResult advance(ObjectsLoadState& state, bool mainThread = false);

    // autoreleases the object and returns it
    GameObject* takeOutObject();

    // returns whether this object is ok with polymorphic init being out of order
    bool needsOrderedPolyInit();

private:
    geode::Ref<GameObject> m_object = nullptr;
    std::string* m_source = nullptr;
    ObjectProps<true> m_props;
    int m_spriteKey;
    uint32_t m_index;
    std::atomic<State> m_state;
    bool m_autoreleased = false;
    bool m_needsCustomSetup = true;
    bool m_needsSetupSprites = true;
    // bool m_needsPolyInitOrder = true;

    void cleanup();
    bool autoreleased();

    void parseProps(ObjectsLoadState& state);
    void createObjectWithKey(int objectId);
    void createObjectCustom(int objectId);

    void loadBasicProperties(ObjectsLoadState& state);
    void doPolymorphicInit(ObjectsLoadState& state);
    void loadRemainingProperties(ObjectsLoadState& state);
    void performPostInit(ObjectsLoadState& state);
    void performFinalCleanup();

    // some vanilla reimpls
    void loadGroupsFromString(std::string_view sv);
    void addColorSprite(std::string_view sv);
    void addGlow(std::string_view sv);
};

}
