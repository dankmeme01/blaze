#pragma once
#include <Geode/Geode.hpp>
#include <util/macros.hpp>
#include <util/FlatHashMap.hpp>

// Defines the size of the prop vector, e.g. 606 means indices from 0 to 605 are valid prop numbers (though 0 is never used)
static constexpr size_t MAX_OBJECT_PROPS = 606;
static constexpr size_t HIGHEST_PROP_ID = MAX_OBJECT_PROPS - 1;

namespace blaze {

template <bool Compact>
struct ObjectProps;

template <>
struct ObjectProps<false> {
    std::vector<std::string> m_values;
    std::vector<void*> m_present;

    ObjectProps(const ObjectProps&) = delete;
    ObjectProps& operator=(const ObjectProps&) = delete;
    ObjectProps(ObjectProps&& other) noexcept {
        *this = std::move(other);
    }

    ObjectProps& operator=(ObjectProps&& other) noexcept {
        if (this != &other) {
            m_values = std::move(other.m_values);
            m_present = std::move(other.m_present);
        }
        return *this;
    }

    ObjectProps() {
        m_values.resize(MAX_OBJECT_PROPS);
        m_present.resize(MAX_OBJECT_PROPS);
    }

    void reset() {
        for (auto& p : m_present) {
            p = nullptr;
        }
    }

    /// Sets the property, key must be in bounds
    void set(int key, std::string_view value) {
        B_DEBUG_ASSERT(key >= 0 && key <= HIGHEST_PROP_ID && this->valid());
        m_values[key] = std::string{value.data(), value.size()};
        m_present[key] = this;
    }

    /// Note: keys must be in bounds
    bool has(int key) const {
        B_DEBUG_ASSERT(key >= 0 && key <= HIGHEST_PROP_ID && this->valid());
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

private:
    bool valid() const {
        return !m_values.empty();
    }
};

template <>
struct ObjectProps<true> {
    FlatHashMap<int, std::string_view> m_values;

    ObjectProps() {}

    void reset() {
        m_values.clear();
    }

    /// Sets the property, key must be in bounds
    void set(int key, std::string_view value) {
        B_DEBUG_ASSERT(key >= 0 && key <= HIGHEST_PROP_ID);
        m_values[key] = value;
    }

    /// Note: keys must be in bounds
    bool has(int key) const {
        B_DEBUG_ASSERT(key >= 0 && key <= HIGHEST_PROP_ID);
        return m_values.contains(key);
    }

    int getInt(int key) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? geode::utils::numFromString<int>(it->second).unwrapOr(0) : 0;
    }

    float getFloat(int key) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? geode::utils::numFromString<float>(it->second).unwrapOr(0) : 0;
    }

    float getFloatFinite(int key) const {
        float val = getFloat(key);
        return std::isfinite(val) ? val : 0.0f;
    }

    bool getBool(int key) const {
        // optimized path
        auto it = m_values.find(key);
        return it != m_values.end() && !this->looksLikeZero(it->second);
    }

    std::string_view getString(int key) const {
        auto it = m_values.find(key);
        return it != m_values.end() ? it->second : "";
    }

    void fillNonCompact(ObjectProps<false>& other) const {
        for (auto [k, v] : m_values) {
            other.set(k, v);
        }
    }

private:
    bool looksLikeZero(std::string_view sv) const {
        if (sv.empty()) return true; // empty string
        if (sv.front() == '-' || sv.front() == '+') sv.remove_prefix(1);
        if (sv.empty()) return true; // bare + or -

        for (char c : sv) {
            if (c != '0') return false;
        }
        return true; // only zeroes
    }
};

}
