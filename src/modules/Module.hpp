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

    template <typename... HookNames>
    void addHooks(auto& modify, HookNames... names) {
        (addHookHelper(modify, names), ...);
    }

    void addHook(geode::Hook* hook) {
        m_hooks.push_back(hook);
    }

private:
    friend Derived;
    Module() = default;

    // automatically initialize the module when the program starts
    static inline ModuleCrtpAutoInit<Derived> s_autoInit;
    static inline auto s_autoInitRef = &Module::s_autoInit;

    std::vector<geode::Hook*> m_hooks;

    void addHookHelper(auto& modify, auto hookName) {
        auto res = modify.getHook(hookName);
        if (!res) {
            geode::log::warn("Missing hook: {} for {}", hookName, arc::getTypename<Derived>());
        } else {
            this->addHook(res.unwrap());
        }
    }
};

}
