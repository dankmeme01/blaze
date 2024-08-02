#include "TaskTimer.hpp"

#include <Geode/loader/Log.hpp>

using namespace geode::prelude;

static std::string formatMeasurement(std::string_view name, std::chrono::nanoseconds timeTook) {
    return fmt::format("{} - {}", name, formatDuration(timeTook));
}

std::string TaskTimer::Summary::format() {
    std::string out;

    for (auto& [name, timeTook] : measurements) {
        out += formatMeasurement(name, timeTook);
        out += "\n";
    }

    return out;
}

void TaskTimer::Summary::print() {
    log::debug("==================================");
    for (auto& [name, timeTook] : measurements) {
        log::debug("{}", formatMeasurement(name, timeTook));
    }
    log::debug("==================================");
}

TaskTimer::TaskTimer(std::string_view stepName) {
    this->step(stepName);
}

TaskTimer::~TaskTimer() {
    if (measurements.size()) {
        this->finish().print();
    }
}

void TaskTimer::step(std::string_view stepName) {
    auto timer = std::chrono::high_resolution_clock::now();

    if (currentStep.empty()) {
        this->currentStep = stepName;
        this->currentStepTime = timer;
        return;
    }

    auto passed = std::chrono::duration_cast<std::chrono::nanoseconds>(timer - this->currentStepTime);
    auto measurement = std::make_pair(currentStep, passed);

    this->currentStepTime = timer;
    this->currentStep = stepName;

    measurements.emplace_back(std::move(measurement));
}

TaskTimer::Summary TaskTimer::finish() {
    this->step("");

    Summary summary(std::move(measurements));
    this->reset();
    return summary;
}

void TaskTimer::reset() {
    measurements.clear();
    currentStep.clear();
}