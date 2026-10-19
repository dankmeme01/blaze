#pragma once
#include <vector>
#include <utility>
#include <algorithm>

namespace blaze {

/// Binary search based flat hash map
template <typename K, typename V>
class FlatHashMap {
public:
    using iterator = typename std::vector<std::pair<K, V>>::iterator;

    FlatHashMap() = default;

    void reserve(size_t n) {
        m_values.reserve(n);
    }

    bool empty() const {
        return m_values.empty();
    }

    size_t size() const {
        return m_values.size();
    }

    auto find(this auto& self, const K& key) {
        auto it = self.lowerBound(key);
        if (it != self.m_values.end() && it->first == key) {
            return it;
        }
        return self.m_values.end();
    }

    auto begin(this auto& self) {
        return self.m_values.begin();
    }

    auto end(this auto& self) {
        return self.m_values.end();
    }

    bool contains(const K& key) const {
        auto it = lowerBound(key);
        return it != m_values.end() && it->first == key;
    }

    auto& at(this auto& self, const K& key) {
        auto it = self.lowerBound(key);
        if (it != self.m_values.end() && it->first == key) {
            return it->second;
        }
        throw std::out_of_range("key not found");
    }

    V& operator[](const K& key) {
        auto it = lowerBound(key);
        if (it != m_values.end() && it->first == key) {
            return it->second;
        }
        // insert new element
        return m_values.insert(it, {key, V{}})->second;
    }

    std::pair<iterator, bool> insert(const std::pair<K, V>& pair) {
        auto it = lowerBound(pair.first);
        if (it != m_values.end() && it->first == pair.first) {
            return {it, false};
        }
        return {m_values.insert(it, pair), true};
    }

    void erase(const K& key) {
        auto it = lowerBound(key);
        if (it != m_values.end() && it->first == key) {
            m_values.erase(it);
        }
    }

    void clear() {
        m_values.clear();
    }

private:
    std::vector<std::pair<K, V>> m_values;

    auto lowerBound(this auto& self, const K& k) {
        return std::ranges::lower_bound(self.m_values, k, [](const K& a, const K& b) {
            return a < b;
        }, &std::pair<K, V>::first);
    }

    auto upperBound(this auto& self, const K& k) {
        return std::ranges::upper_bound(self.m_values, k, [](const K& a, const K& b) {
            return a < b;
        }, &std::pair<K, V>::first);
    }
};

}

