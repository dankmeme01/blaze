#pragma once
#include <Geode/Geode.hpp>
#include <Geode/modify/GameObject.hpp>
#include <util/macros.hpp>
#include "GameModule.hpp"

namespace blaze {

struct ObjectProps {
    gd::vector<gd::string> m_values;
    gd::vector<void*> m_present;

    ObjectProps() {
        m_values.resize(601);
        m_present.resize(601);
    }

    void reset() {
        for (auto& p : m_present) {
            p = nullptr;
        }
    }

    /// Sets the property, key must be in bounds (0..=600)
    void set(int key, std::string_view value) {
        B_DEBUG_ASSERT(key >= 0 && key <= 600);
        m_values[key] = gd::string{value.data(), value.size()};
        m_present[key] = this;
    }

    /// Note: keys must be in bounds (0..=600)
    bool has(int key) const {
        B_DEBUG_ASSERT(key >= 0 && key <= 600);
        return m_present[key] != nullptr;
    }

    int getInt(int key) const {
        return has(key) ? geode::utils::numFromString<int>(m_values[key]).unwrapOr(0) : 0;
    }

    float getFloat(int key) const {
        return has(key) ? geode::utils::numFromString<float>(m_values[key]).unwrapOr(0) : 0;
    }

    float getFloatFinite(int key) const {
        float val = getFloat(key);
        return std::isfinite(val) ? val : 0.0f;
    }

    bool getBool(int key) const {
        return getInt(key) != 0;
    }

    std::string_view getString(int key) const {
        return has(key) ? std::string_view{m_values[key].data(), m_values[key].size()} : std::string_view{};
    }
};

struct BlazeGameObject : geode::Modify<BlazeGameObject, GameObject> {
    static GameObject* fromValues(ObjectProps& props, GJBaseGameLayer* gameLayer, bool lowDetail);

    static void onModify(auto& self) {
        GameModule::get().addHooks(self);

        (void) self.setHookPriority("GameObject::createWithFrame", geode::Priority::Replace);
    }

    $override
    static GameObject* createWithFrame(char const* name);

    $override
    void loadGroupsFromString(gd::string groupList);

    $override
    void createGroupContainer(int size);
};

}
