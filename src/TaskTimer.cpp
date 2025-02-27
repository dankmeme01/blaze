#include "TaskTimer.hpp"

#include <Geode/loader/Log.hpp>

using namespace geode::prelude;
using namespace asp::time;

static std::string formatMeasurement(std::string_view name, asp::time::Duration timeTook) {
    return fmt::format("{} - {}", name, timeTook.toString());
}

std::string TaskTimer::Summary::format() {
    std::string out;

    for (auto& [name, timeTook] : measurements) {
        out += formatMeasurement(name, timeTook);
        out += "\n";
    }
    out += fmt::format("Total time: {}\n", totalTime.toString());

    return out;
}

void TaskTimer::Summary::print() {
    log::debug("==================================");
    for (auto& [name, timeTook] : measurements) {
        log::debug("{}", formatMeasurement(name, timeTook));
    }
    log::debug("Total time: {}", totalTime.toString());
    log::debug("==================================");
}

TaskTimer::TaskTimer(std::string_view stepName) {
    this->step(stepName);
}

TaskTimer::~TaskTimer() {
    if (!currentStep.empty()) {
        this->step("");
    }

    if (measurements.size()) {
        this->finish().print();
    }
}

void TaskTimer::step(std::string_view stepName) {
    auto timer = Instant::now();

    if (currentStep.empty()) {
        this->currentStep = stepName;
        this->currentStepTime = timer;
        this->firstStepTime = timer;
        return;
    }

    auto passed = timer.durationSince(this->currentStepTime);
    auto measurement = std::make_pair(currentStep, passed);

    this->currentStepTime = timer;
    this->currentStep = stepName;

    measurements.emplace_back(std::move(measurement));
}

TaskTimer::Summary TaskTimer::finish() {
    if (!currentStep.empty()) {
        this->step("");
    }

    Summary summary(std::move(measurements), this->firstStepTime.elapsed());
    this->reset();
    return summary;
}

void TaskTimer::reset() {
    measurements.clear();
    currentStep.clear();
}