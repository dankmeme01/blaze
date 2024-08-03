#pragma once

#include <util.hpp>
#include <string>
#include <chrono>

#ifdef BLAZE_DEBUG
#define BLAZE_TIMER_START(name) ::TaskTimer __task_timer(name)
#define BLAZE_TIMER_STEP(name) __task_timer.step(name)
#define BLAZE_TIMER_END() __task_timer.finish().print()
#else
#define BLAZE_TIMER_START(name) (void)0
#define BLAZE_TIMER_STEP(name) (void)0
#define BLAZE_TIMER_END(name) (void)0
#endif

class TaskTimer {
    std::vector<std::pair<std::string, std::chrono::nanoseconds>> measurements;
    std::string currentStep;
    std::chrono::high_resolution_clock::time_point currentStepTime;
    std::chrono::high_resolution_clock::time_point firstStepTime;

public:
    TaskTimer(std::string_view name);
    ~TaskTimer();

    struct Summary {
        std::vector<std::pair<std::string, std::chrono::nanoseconds>> measurements;
        std::chrono::nanoseconds totalTime;

        std::string format();
        void print();

        Summary(const decltype(measurements)& m, std::chrono::nanoseconds total) : measurements(m), totalTime(total) {}
        Summary(decltype(measurements)&& m, std::chrono::nanoseconds total) : measurements(std::move(m)), totalTime(total) {}
    };

    void step(std::string_view stepName);
    Summary finish();
    void reset();
};