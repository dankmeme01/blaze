#pragma once

template <typename Derived>
class SingletonBase {
public:
    // no copy
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    // no move
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

    static Derived& get() {
        static Derived instance;

        return instance;
    }

protected:
    SingletonBase() {}
    virtual ~SingletonBase() {}
};

// example: 2.123s, 69.123ms
template <typename Rep, typename Period>
std::string formatDuration(const std::chrono::duration<Rep, Period>& time) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time).count();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(time).count();

    if (seconds > 0) {
        return std::to_string(seconds) + "." + std::to_string(millis % 1000) + "s";
    } else if (millis > 0) {
        return std::to_string(millis) + "." + std::to_string(micros % 1000) + "ms";
    } else {
        return std::to_string(micros) + "μs";
    }
}

inline std::chrono::nanoseconds benchTimer() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
}
