#pragma once
#include <chrono>
#include <iostream>

#ifdef TIMING
#define INIT_TIMER(name)          Timer timer_##name{#name};
#define MESSAGE_TIMER(name, msg)  Timer timer_##name{msg};
#define STOP_TIMER(name)          timer_##name.end();
#else
#define INIT_TIMER(name)
#define MESSAGE_TIMER(name, message)
#define STOP_TIMER(name)
#endif

class Timer {
public:
    explicit Timer(const char *name) : name(name), start_(now()), end_(0) {
    }

    ~Timer() {
        end();
    }

    void end() {
        if (end_ != 0) return;
        end_ = now();
        std::cout << "Timer:" << name << ", Elapsed time: " << (end_ - start_) << " ms" << std::endl;
    }

private:
    static long now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                .count();
    }

    const char *name;

    long start_;
    long end_;
};
