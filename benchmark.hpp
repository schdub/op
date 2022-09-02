#pragma once

#include <chrono>

namespace op {

class benchmark {
private:
    const char * m;
    std::chrono::high_resolution_clock::time_point t1, t2;
public:
    explicit benchmark(const char* msg=nullptr) : m(msg) {
        start();
    }
    ~benchmark() {
        stop();
        if (m) std::cout << m << " ";
        std::cout << msec_elapsed() << std::endl;
    }
    inline void start() {
        t1 = std::chrono::high_resolution_clock::now();
    }
    inline void stop() {
        t2 = std::chrono::high_resolution_clock::now();
    }
    inline unsigned msec_elapsed() {
        return std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    }
};

#define OP_BENCHMARK op::benchmark __op_bench##COUNTER(__FUNCTION__)

};