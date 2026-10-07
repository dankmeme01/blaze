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
            m_hooks.push_back(hook.get());
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
