#pragma once

#include <util.hpp>
#include <string>
#include <chrono>

#ifdef BLAZE_DEBUG
#define BLAZE_TIMER_START(name) ::TaskTimer::get().start(name)
#define BLAZE_TIMER_STEP(name) ::TaskTimer::get().step(name)
#define BLAZE_TIMER_END() ::TaskTimer::get().finish().print()
#else
#define BLAZE_TIMER_START(name) (void)0
#define BLAZE_TIMER_STEP(name) (void)0
#define BLAZE_TIMER_END(name) (void)0
#endif

class TaskTimer : public SingletonBase<TaskTimer> {
    friend class SingletonBase;
    std::vector<std::pair<std::string, std::chrono::nanoseconds>> measurements;
    std::string currentStep;
    std::chrono::high_resolution_clock::time_point currentStepTime;

public:
    struct Summary {
        std::vector<std::pair<std::string, std::chrono::nanoseconds>> measurements;

        std::string format();
        void print();

        Summary(const decltype(measurements)& m) : measurements(m) {}
        Summary(decltype(measurements)&& m) : measurements(std::move(m)) {}
    };

    void start(std::string_view stepName);
    void step(std::string_view stepName);
    Summary finish();
    void reset();
};