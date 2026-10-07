#pragma once

#include <Geode/Geode.hpp>
#include <arc/future/PollableMetadata.hpp>

namespace blaze {

template <typename Derived>
struct ModuleCrtpAutoInit {
    ModuleCrtpAutoInit() {
        Derived::get();
    }
};

template <typename Derived>
struct Module {
    static Derived& get() {
        static Derived instance;
        return instance;
    }

    void addHooks(auto& modify) {
        for (auto& [k, hook] : modify.m_hooks) {
            auto res = modify.getHook(k);
            if (!res) {
                auto n = arc::getTypename<Derived>();
                geode::log::warn("Missing hook: {} for {}", k, std::string_view{n.first, n.second});
            } else {
                m_hooks.push_back(res);
            }
        }
    }

private:
    friend Derived;
    Module() = default;

    // automatically initialize the module when the program starts
    static inline ModuleCrtpAutoInit<Derived> s_autoInit;
    static inline auto s_autoInitRef = &Module::s_autoInit;

    std::vector<geode::Hook*> m_hooks;
};

}
